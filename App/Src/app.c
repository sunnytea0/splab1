/* ЛР3: super loop з мітками часу + вимірювальний код. */
#include "app.h"
#include "mpu6050.h"
#include "button.h"
#include "perf.h"
#include "perf_stat.h"

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#ifndef GIT_HASH
#define GIT_HASH "unknown"
#endif

#define BUTTON_DEBOUNCE_MS   50U
#define CMD_LINE_MAX         48U
#define CMD_BYTES_PER_CALL   16U

/* ---------------- Періодичні задачі ---------------- */

typedef struct {
    const char *name;
    uint32_t    period_us;   /* 0 = вимкнено */
    uint32_t    next_us;
    uint32_t    runs;
    uint32_t    missed;
    perf_stat_t exec;
    perf_stat_t late;
    perf_hist_t hist;
} ptask_t;

static const uint32_t LATE_EDGES[PERF_HIST_BINS - 1U] =
    { 100U, 500U, 1000U, 5000U, 20000U, 50000U, 100000U };

static void ptask_init(ptask_t *t, const char *name, uint32_t period_us, uint32_t now)
{
    memset(t, 0, sizeof(*t));
    t->name      = name;
    t->period_us = period_us;
    t->next_us   = now + period_us;
    perf_hist_init(&t->hist, LATE_EDGES);
}

static void ptask_set_period(ptask_t *t, uint32_t period_us, uint32_t now)
{
    t->period_us = period_us;
    t->next_us   = now + period_us;
}

static void ptask_reset_stats(ptask_t *t)
{
    t->runs = 0U;
    t->missed = 0U;
    perf_stat_reset(&t->exec);
    perf_stat_reset(&t->late);
    perf_hist_reset(&t->hist);
}

static int ptask_due(ptask_t *t, uint32_t now)
{
    if (t->period_us == 0U) { return 0; }

    int32_t d = (int32_t)(now - t->next_us);
    if (d < 0) { return 0; }

    uint32_t late = (uint32_t)d;
    perf_stat_add(&t->late, late);
    perf_hist_add(&t->hist, late);

    if (late >= t->period_us)
    {
        t->missed  += late / t->period_us;
        t->next_us  = now + t->period_us;
    }
    else
    {
        t->next_us += t->period_us;
    }
    t->runs++;
    return 1;
}

/* ---------------- Стан застосунку ---------------- */

static ADC_HandleTypeDef  *app_adc  = NULL;
static UART_HandleTypeDef *app_uart = NULL;
static MPU6050_Handle      mpu6050;
static Button              button;

static ptask_t t_imu, t_adc, t_report, t_long, t_autostats;

static uint8_t  mode = 0U;

static MPU6050_Data imu_data;
static uint8_t      imu_ok = 0U;
static uint32_t     imu_errors = 0U;
static int32_t      prev_mg[3];
static uint8_t      have_prev = 0U;
static uint32_t     motion_events = 0U;
static uint32_t     threshold_mg = 0U;
static uint32_t     adc_last = 0U;

static volatile uint32_t btn_t_event_us = 0U;
static volatile uint8_t  btn_pending = 0U;
static volatile uint32_t btn_last_ms = 0U;
static volatile uint8_t  btn_have_last = 0U;
static perf_stat_t       st_btn_lat;
static perf_hist_t       h_btn_lat;

static char     cmd_buf[CMD_LINE_MAX];
static uint32_t cmd_len = 0U;
static uint32_t rx_bytes = 0U;
static uint32_t rx_overruns = 0U;
static perf_stat_t st_cmd;

static perf_stat_t st_loop;
static uint32_t    loop_prev_us = 0U;
static uint32_t    busy_us = 0U;
static uint32_t    win_start_us = 0U;
static uint32_t    load_pct = 0U;
static uint32_t    load_max_pct = 0U;

static uint32_t long_dur_us = 0U;

static uint8_t  scen_id = 0U;
static uint32_t scen_end_us = 0U;
static uint8_t  scen_active = 0U;

/* ---------------- Допоміжне ---------------- */

static void busy_wait_us(uint32_t dur_us)
{
    uint32_t t0 = perf_us();
    while ((uint32_t)(perf_us() - t0) < dur_us) { }
}

static uint32_t App_ReadADC(void)
{
    uint32_t value = 0;

    if (HAL_ADC_Start(app_adc) != HAL_OK) { return 0; }
    if (HAL_ADC_PollForConversion(app_adc, 100) == HAL_OK)
    {
        value = HAL_ADC_GetValue(app_adc);
    }
    HAL_ADC_Stop(app_adc);
    return value;
}

/* Кнопка між PB0 і GND + pull-up: натиснута = 0. */
static int App_ReadButtonPressed(void)
{
    return (HAL_GPIO_ReadPin(GPIOB, GPIO_PIN_0) == GPIO_PIN_RESET) ? 1 : 0;
}

static void stats_reset_all(uint32_t now)
{
    ptask_reset_stats(&t_imu);
    ptask_reset_stats(&t_adc);
    ptask_reset_stats(&t_report);
    ptask_reset_stats(&t_long);
    ptask_reset_stats(&t_autostats);
    perf_stat_reset(&st_btn_lat);
    perf_hist_reset(&h_btn_lat);
    perf_stat_reset(&st_cmd);
    perf_stat_reset(&st_loop);
    imu_errors = 0U;
    motion_events = 0U;
    rx_bytes = 0U;
    rx_overruns = 0U;
    busy_us = 0U;
    load_max_pct = 0U;
    win_start_us = now;
}

/* ---------------- ISR-частина кнопки ---------------- */

void App_OnButtonPressed(uint32_t now_ms)
{
    /* Мітка часу лише для першого фронту після паузи >= debounce. */
    if (!btn_have_last || ((uint32_t)(now_ms - btn_last_ms) >= BUTTON_DEBOUNCE_MS))
    {
        btn_last_ms   = now_ms;
        btn_have_last = 1U;
        btn_t_event_us = perf_us();
        btn_pending    = 1U;
    }
    (void)Button_OnFallingEdge(&button, now_ms);   /* ISR context */
}

/* ---------------- Self-test (як у ЛР2) ---------------- */

static void App_SelfTest(void)
{
    uint8_t all_ok = 1;
    uint8_t who_am_i = 0;
    MPU6050_Data test_data;

    printf("\r\n====================================\r\n");
    printf("[SELFTEST] START\r\n");
    printf("====================================\r\n");

    if (MPU6050_CheckConnection(&mpu6050) == HAL_OK)
    {
        printf("[SELFTEST] I2C MPU6050 ADDRESS=0x68 OK\r\n");
    }
    else
    {
        printf("[SELFTEST] I2C MPU6050 NOT FOUND\r\n");
        all_ok = 0;
    }

    HAL_StatusTypeDef who_status = MPU6050_ReadWhoAmI(&mpu6050, &who_am_i);
    if (who_status == HAL_OK)
    {
        printf("[SELFTEST] WHO_AM_I=0x%02X", who_am_i);
        if (who_am_i == MPU6050_EXPECTED_ID)
        {
            printf(" OK\r\n");
        }
        else
        {
            printf(" FAIL expected=0x%02X\r\n", MPU6050_EXPECTED_ID);
            all_ok = 0;
        }
    }
    else
    {
        printf("[SELFTEST] WHO_AM_I READ FAIL status=%d\r\n", (int)who_status);
        all_ok = 0;
    }

    HAL_StatusTypeDef data_status = MPU6050_ReadData(&mpu6050, &test_data);
    if (data_status == HAL_OK)
    {
        printf("[SELFTEST] MPU6050 MULTIBYTE READ OK\r\n");
        printf("[SELFTEST] RAW AX=%d AY=%d AZ=%d TEMP=%d GX=%d GY=%d GZ=%d\r\n",
               test_data.accel_x, test_data.accel_y, test_data.accel_z,
               test_data.temperature,
               test_data.gyro_x, test_data.gyro_y, test_data.gyro_z);
    }
    else
    {
        printf("[SELFTEST] MPU6050 MULTIBYTE READ FAIL status=%d\r\n", (int)data_status);
        all_ok = 0;
    }

    printf("[SELFTEST] ADC VALUE=%lu OK\r\n", (unsigned long)App_ReadADC());
    printf("[SELFTEST] BUTTON PRESSED=%d OK (EXTI0, falling edge)\r\n", App_ReadButtonPressed());
    printf("[SELFTEST] UART OK\r\n");
    printf("------------------------------------\r\n");
    printf(all_ok ? "[SELFTEST] RESULT=PASS\r\n" : "[SELFTEST] RESULT=FAIL\r\n");
    printf("====================================\r\n");
}

/* ---------------- Звіти вимірювань ---------------- */

static void print_ptask(const ptask_t *t)
{
    printf("STAT name=%s n=%lu min_us=%lu avg_us=%lu max_us=%lu\r\n",
           t->name, (unsigned long)t->exec.count,
           (unsigned long)t->exec.min, (unsigned long)perf_stat_avg(&t->exec),
           (unsigned long)t->exec.max);

    if (t->period_us != 0U)
    {
        printf("JIT name=%s period_us=%lu n=%lu late_min_us=%lu late_avg_us=%lu "
               "late_max_us=%lu missed=%lu\r\n",
               t->name, (unsigned long)t->period_us, (unsigned long)t->late.count,
               (unsigned long)t->late.min, (unsigned long)perf_stat_avg(&t->late),
               (unsigned long)t->late.max, (unsigned long)t->missed);
    }
}

static void print_hist(const char *name, const perf_hist_t *h)
{
    printf("HIST name=%s edges_us=%lu,%lu,%lu,%lu,%lu,%lu,%lu bins=",
           name,
           (unsigned long)h->edge[0], (unsigned long)h->edge[1],
           (unsigned long)h->edge[2], (unsigned long)h->edge[3],
           (unsigned long)h->edge[4], (unsigned long)h->edge[5],
           (unsigned long)h->edge[6]);
    for (uint32_t i = 0U; i < PERF_HIST_BINS; i++)
    {
        printf("%lu%s", (unsigned long)h->bin[i], (i + 1U < PERF_HIST_BINS) ? "," : "\r\n");
    }
}

static void print_stats(void)
{
    printf("FW version=%s scen=%u mode=%u\r\n", GIT_HASH, (unsigned)scen_id, (unsigned)mode);
    print_ptask(&t_imu);
    print_ptask(&t_adc);
    print_ptask(&t_report);
    print_ptask(&t_long);

    printf("STAT name=cmd n=%lu min_us=%lu avg_us=%lu max_us=%lu\r\n",
           (unsigned long)st_cmd.count, (unsigned long)st_cmd.min,
           (unsigned long)perf_stat_avg(&st_cmd), (unsigned long)st_cmd.max);

    printf("STAT name=loop n=%lu min_us=%lu avg_us=%lu max_us=%lu\r\n",
           (unsigned long)st_loop.count, (unsigned long)st_loop.min,
           (unsigned long)perf_stat_avg(&st_loop), (unsigned long)st_loop.max);

    printf("LAT name=btn n=%lu min_us=%lu avg_us=%lu max_us=%lu\r\n",
           (unsigned long)st_btn_lat.count, (unsigned long)st_btn_lat.min,
           (unsigned long)perf_stat_avg(&st_btn_lat), (unsigned long)st_btn_lat.max);

    printf("LOAD pct_last=%lu pct_max=%lu\r\n",
           (unsigned long)load_pct, (unsigned long)load_max_pct);
    printf("RX bytes=%lu overruns=%lu\r\n", (unsigned long)rx_bytes, (unsigned long)rx_overruns);
    printf("MOTION events=%lu threshold_mg=%lu imu_errors=%lu\r\n",
           (unsigned long)motion_events, (unsigned long)threshold_mg,
           (unsigned long)imu_errors);
}

static void print_mem(void)
{
    printf("MEM stack_used=%lu stack_reserved=%lu heap_used=%lu\r\n",
           (unsigned long)stack_used_bytes(), (unsigned long)stack_reserved_bytes(),
           (unsigned long)heap_used_bytes());
}

/* ---------------- Сценарії S1–S3 ---------------- */

static void scen_start(uint8_t id, uint32_t seconds)
{
    uint32_t now = perf_us();

    if (id < 1U || id > 3U) { printf("ERR scen 1..3\r\n"); return; }

    /* S1: IMU 50 Гц, звіт 1 Гц
     * S2: IMU 100 Гц, звіт 1 Гц, STATS раз на 5 с (потік команд)
     * S3: IMU 100 Гц, звіт 5 Гц, довга операція 100 мс кожні 500 мс */
    ptask_set_period(&t_imu,    (id == 1U) ? 20000U : 10000U, now);
    ptask_set_period(&t_adc,    100000U, now);
    ptask_set_period(&t_report, (id == 3U) ? 200000U : 1000000U, now);
    ptask_set_period(&t_long,   (id == 3U) ? 500000U : 0U, now);
    ptask_set_period(&t_autostats, (id == 2U) ? 5000000U : 0U, now);
    long_dur_us = (id == 3U) ? 100000U : 0U;

    stats_reset_all(now);
    scen_id = id;
    scen_active = 1U;
    scen_end_us = now + seconds * 1000000U;

    printf("SCEN_START id=%u seconds=%lu fw=%s\r\n", (unsigned)id, (unsigned long)seconds, GIT_HASH);
}

static void scen_finish(void)
{
    scen_active = 0U;
    printf("SCEN_END id=%u\r\n", (unsigned)scen_id);
    print_stats();
    print_hist("imu_late", &t_imu.hist);
    print_hist("btn_latency", &h_btn_lat);
    print_mem();
    printf("SCEN_DONE id=%u\r\n", (unsigned)scen_id);
     /* Повернути задачі у стан за замовчуванням (інакше longop/autostats
     * продовжують працювати після кінця сценарію). */
    {
        uint32_t t_now = perf_us();
        ptask_set_period(&t_imu,       20000U,   t_now);
        ptask_set_period(&t_adc,       100000U,  t_now);
        ptask_set_period(&t_report,    1000000U, t_now);
        ptask_set_period(&t_long,      0U,       t_now);
        ptask_set_period(&t_autostats, 0U,       t_now);
        long_dur_us = 0U;
    }
#ifdef AUTORUN_SCEN
    if (scen_id < 3U) { scen_start((uint8_t)(scen_id + 1U), 30U); }
#endif
}

/* ---------------- Командний інтерфейс ---------------- */

static uint32_t arg_u32(const char *s, uint32_t idx)
{
    while (idx > 0U)
    {
        while ((*s != '\0') && (*s != ' ')) { s++; }
        while (*s == ' ') { s++; }
        idx--;
    }
    return (uint32_t)strtoul(s, NULL, 10);
}

static void cmd_exec(char *line)
{
    char *args = line;
    while ((*args != '\0') && (*args != ' ')) { args++; }
    if (*args != '\0') { *args++ = '\0'; }
    while (*args == ' ') { args++; }

    uint32_t now = perf_us();

    if      (strcmp(line, "STATS") == 0)  { print_stats(); }
    else if (strcmp(line, "RESET") == 0)  { stats_reset_all(now); printf("OK reset\r\n"); }
    else if (strcmp(line, "HIST") == 0)   { print_hist("imu_late", &t_imu.hist);
                                            print_hist("btn_latency", &h_btn_lat); }
    else if (strcmp(line, "MEM") == 0)    { print_mem(); }
    else if (strcmp(line, "VER") == 0)    { printf("FW version=%s\r\n", GIT_HASH); }
    else if (strcmp(line, "SELFTEST") == 0) { App_SelfTest(); }
    else if (strcmp(line, "IMU") == 0)    { ptask_set_period(&t_imu, arg_u32(args, 0) * 1000U, now);
                                            printf("OK imu period_ms=%lu\r\n", (unsigned long)arg_u32(args, 0)); }
    else if (strcmp(line, "REPORT") == 0) { ptask_set_period(&t_report, arg_u32(args, 0) * 1000U, now);
                                            printf("OK report period_ms=%lu\r\n", (unsigned long)arg_u32(args, 0)); }
    else if (strcmp(line, "LONG") == 0)   { ptask_set_period(&t_long, arg_u32(args, 0) * 1000U, now);
                                            long_dur_us = arg_u32(args, 1) * 1000U;
                                            printf("OK long period_ms=%lu dur_ms=%lu\r\n",
                                                   (unsigned long)arg_u32(args, 0),
                                                   (unsigned long)arg_u32(args, 1)); }
    else if (strcmp(line, "BUSY") == 0)   { PROBE_ON(PROBE_LONG_PIN);
                                            busy_wait_us(arg_u32(args, 0) * 1000U);
                                            PROBE_OFF(PROBE_LONG_PIN);
                                            printf("OK busy done\r\n"); }
    else if (strcmp(line, "SCEN") == 0)   { uint32_t s = arg_u32(args, 1);
                                            scen_start((uint8_t)arg_u32(args, 0), (s == 0U) ? 30U : s); }
    else if (strcmp(line, "HELP") == 0)   { printf("CMDS: STATS RESET HIST MEM VER SELFTEST IMU <ms> "
                                                   "REPORT <ms|0> LONG <period_ms> <dur_ms> BUSY <ms> "
                                                   "SCEN <1..3> [sec]\r\n"); }
    else if (line[0] != '\0')             { printf("ERR unknown cmd: %s\r\n", line); }
}

/* Опитування UART без переривань. Байти, що прийшли під час довгої
 * операції, втрачаються (ORE) — це межа підходу, рахуємо в rx_overruns. */
static uint8_t task_cmd(void)
{
    UART_HandleTypeDef *u = app_uart;
    uint8_t worked = 0U;

    if (__HAL_UART_GET_FLAG(u, UART_FLAG_ORE) != RESET)
    {
        __HAL_UART_CLEAR_OREFLAG(u);
        rx_overruns++;
        worked = 1U;
    }

    for (uint32_t n = 0U; n < CMD_BYTES_PER_CALL; n++)
    {
        if (__HAL_UART_GET_FLAG(u, UART_FLAG_RXNE) == RESET) { break; }

        char c = (char)(u->Instance->RDR & 0xFFU);
        rx_bytes++;
        worked = 1U;

        if ((c == '\r') || (c == '\n'))
        {
            if (cmd_len > 0U)
            {
                cmd_buf[cmd_len] = '\0';
                cmd_len = 0U;
                cmd_exec(cmd_buf);
            }
        }
        else if (cmd_len < (CMD_LINE_MAX - 1U))
        {
            cmd_buf[cmd_len++] = c;
        }
        else
        {
            cmd_len = 0U;
        }
    }
    return worked;
}

/* ---------------- Задачі ---------------- */

static void task_imu(void)
{
    PROBE_ON(PROBE_IMU_PIN);
    HAL_StatusTypeDef st = MPU6050_ReadData(&mpu6050, &imu_data);
    PROBE_OFF(PROBE_IMU_PIN);

    if (st != HAL_OK)
    {
        imu_ok = 0U;
        imu_errors++;
        return;
    }
    imu_ok = 1U;

    int32_t mg[3];
    mg[0] = MPU6050_AccelToMg(imu_data.accel_x);
    mg[1] = MPU6050_AccelToMg(imu_data.accel_y);
    mg[2] = MPU6050_AccelToMg(imu_data.accel_z);

    if (have_prev)
    {
        int32_t delta = 0;
        for (int i = 0; i < 3; i++)
        {
            int32_t d = mg[i] - prev_mg[i];
            delta += (d < 0) ? -d : d;
        }
        /* FR-05: поріг із потенціометра, 0..4095 -> 0..~2000 мг */
        threshold_mg = (adc_last * 1000U) >> 11U;
        if ((uint32_t)delta > threshold_mg) { motion_events++; }
    }
    for (int i = 0; i < 3; i++) { prev_mg[i] = mg[i]; }
    have_prev = 1U;
}

static void task_adc(void)
{
    adc_last = App_ReadADC();
}

static void task_button(void)
{
    if (!btn_pending) { return; }
    btn_pending = 0U;

    uint32_t t_ev = btn_t_event_us;
    uint32_t presses = Button_TakeNewPresses(&button);
    if (presses == 0U) { return; }      /* відфільтрований дребезг */

    PROBE_ON(PROBE_BTN_PIN);
    uint32_t lat = perf_us() - t_ev;    /* подія (ISR) -> початок реакції */
    perf_stat_add(&st_btn_lat, lat);
    perf_hist_add(&h_btn_lat, lat);

    mode ^= (uint8_t)(presses & 1U);    /* FR-07: перемикання режиму ... */
    motion_events = 0U;                 /* ... і скидання стану          */

    printf("[EVENT] BUTTON PRESSED x%lu | MODE=%u | LAT_us=%lu\r\n",
           (unsigned long)presses, (unsigned)mode, (unsigned long)lat);
    PROBE_OFF(PROBE_BTN_PIN);
}

static void task_report(void)
{
    int button_pressed = App_ReadButtonPressed();   /* FR-06 */

    if (imu_ok)
    {
        printf("MPU OK | ACCEL_mg X=%ld Y=%ld Z=%ld | GYRO_mdps X=%ld Y=%ld Z=%ld | "
               "TEMP_cC=%ld | ADC=%lu | BUTTON_PRESSED=%d | MODE=%u | MOTION=%lu\r\n",
               (long)MPU6050_AccelToMg(imu_data.accel_x),
               (long)MPU6050_AccelToMg(imu_data.accel_y),
               (long)MPU6050_AccelToMg(imu_data.accel_z),
               (long)MPU6050_GyroToMdps(imu_data.gyro_x),
               (long)MPU6050_GyroToMdps(imu_data.gyro_y),
               (long)MPU6050_GyroToMdps(imu_data.gyro_z),
               (long)MPU6050_TemperatureToCentiC(imu_data.temperature),
               (unsigned long)adc_last, button_pressed, (unsigned)mode,
               (unsigned long)motion_events);
    }
    else
    {
        printf("MPU6050 NOT FOUND | MPU READ FAIL | ADC=%lu | BUTTON_PRESSED=%d | MODE=%u\r\n",
               (unsigned long)adc_last, button_pressed, (unsigned)mode);
    }
}

static void task_long(void)
{
    PROBE_ON(PROBE_LONG_PIN);
    busy_wait_us(long_dur_us);
    PROBE_OFF(PROBE_LONG_PIN);
}

/* ---------------- Ініціалізація ---------------- */

void App_Init(I2C_HandleTypeDef *hi2c, ADC_HandleTypeDef *hadc, UART_HandleTypeDef *huart)
{
    app_adc  = hadc;
    app_uart = huart;

    Button_Init(&button, BUTTON_DEBOUNCE_MS);
    MPU6050_Attach(&mpu6050, hi2c);

    printf("\r\n====================================\r\n");
    printf("LAB 3 - SUPER LOOP + MEASUREMENTS\r\n");
    printf("STM32C031C6 + MPU6050 | fw=%s\r\n", GIT_HASH);
    printf("====================================\r\n");

    App_SelfTest();

    uint32_t now = perf_us();
    ptask_init(&t_imu,       "imu",       20000U,   now);
    ptask_init(&t_adc,       "adc",       100000U,  now);
    ptask_init(&t_report,    "report",    1000000U, now);
    ptask_init(&t_long,      "longop",    0U,       now);
    ptask_init(&t_autostats, "autostats", 0U,       now);
    perf_hist_init(&h_btn_lat, LATE_EDGES);
    stats_reset_all(now);
    loop_prev_us = now;

    printf("Type HELP for commands. Wokwi times are SIMULATED.\r\n");

#ifdef AUTORUN_SCEN
    scen_start(1U, 30U);
#endif
}

/* ---------------- Один прохід super loop ---------------- */

void App_Run(void)
{
    uint32_t t0 = perf_us();
    uint32_t t1;

    perf_stat_add(&st_loop, t0 - loop_prev_us);   /* період циклу */
    loop_prev_us = t0;

    if (task_cmd())
    {
        t1 = perf_us();
        perf_stat_add(&st_cmd, t1 - t0);
        busy_us += t1 - t0;
        t0 = t1;
    }

    if (btn_pending)
    {
        task_button();
        t1 = perf_us();
        busy_us += t1 - t0;
        t0 = t1;
    }

    if (ptask_due(&t_imu, t0))
    {
        task_imu();
        t1 = perf_us();
        perf_stat_add(&t_imu.exec, t1 - t0);
        busy_us += t1 - t0;
        t0 = t1;
    }
    if (ptask_due(&t_adc, t0))
    {
        task_adc();
        t1 = perf_us();
        perf_stat_add(&t_adc.exec, t1 - t0);
        busy_us += t1 - t0;
        t0 = t1;
    }
    if (ptask_due(&t_report, t0))
    {
        task_report();
        t1 = perf_us();
        perf_stat_add(&t_report.exec, t1 - t0);
        busy_us += t1 - t0;
        t0 = t1;
    }
    if (ptask_due(&t_long, t0))
    {
        task_long();
        t1 = perf_us();
        perf_stat_add(&t_long.exec, t1 - t0);
        busy_us += t1 - t0;
        t0 = t1;
    }
    if (ptask_due(&t_autostats, t0))
    {
        print_stats();
        t1 = perf_us();
        perf_stat_add(&t_autostats.exec, t1 - t0);
        busy_us += t1 - t0;
        t0 = t1;
    }

    /* завантаження CPU за вікном 1 с */
    if ((uint32_t)(t0 - win_start_us) >= 1000000U)
    {
        load_pct = (busy_us * 100U) / (t0 - win_start_us);
        if (load_pct > load_max_pct) { load_max_pct = load_pct; }
        busy_us = 0U;
        win_start_us = t0;
    }

    /* кінець сценарію */
    if (scen_active && ((int32_t)(t0 - scen_end_us) >= 0))
    {
        scen_finish();
    }
}