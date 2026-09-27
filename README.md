# STM32 Device Position Monitoring System

Лабораторна робота №1 з дисципліни
«Системне програмування вбудованих систем».

## Тема лабораторної роботи

Налаштування середовища розробки та bring-up STM32-платформи.

## Форма виконання

Форма B — симуляція STM32 за допомогою Wokwi.

## Тема проєкту

Система моніторингу положення та керування пристроєм на базі STM32.

## Мікроконтролер

STM32C031C6T6

У симуляторі Wokwi використовується:

`board-st-nucleo-c031c6`

## Компоненти

- MPU-6050 — I2C
- Potentiometer — ADC
- Push Button — GPIO
- USART2 — UART для діагностики

## Підключення

| Компонент | Інтерфейс | STM32 pin |
|---|---|---|
| MPU-6050 SDA | I2C1 | PC14 |
| MPU-6050 SCL | I2C1 | PA9 |
| Potentiometer SIG | ADC1 | PA0 |
| Push Button | GPIO | PB0 |
| USART2 TX | UART | PA2 |
| USART2 RX | UART | PA3 |

## Використані інструменти

- STM32CubeMX
- STM32 HAL
- GNU Arm Embedded Toolchain
- Make
- Docker
- Wokwi for VS Code
- Visual Studio Code

## Збірка

Docker image:

```bash
docker build -t stm32-build docker/.
```

Збірка firmware:

```bash
MSYS_NO_PATHCONV=1 docker run --rm -v "$PWD:/workspace" -w /workspace stm32-build make
```

Після успішної збірки створюється:

```text
build/LR1_.elf
```

## Запуск Wokwi

У Visual Studio Code:

1. Натиснути `F1`.
2. Вибрати `Wokwi: Start Simulator`.

Файл `wokwi.toml` використовує:

```toml
[wokwi]
version = 1
firmware = 'build/LR1_.elf'
elf = 'build/LR1_.elf'
```

## Результат

Під час симуляції перевірено роботу ADC, GPIO, I2C та UART.

Приклад виводу:

```text
MPU6050 detected at address 0x68
ADC = 4095 | BUTTON = 1 | MPU6050 = OK
```

Зміна положення потенціометра змінює значення ADC, натискання кнопки змінює стан BUTTON, а MPU-6050 успішно визначається на I2C-шині за адресою 0x68.

## Документація

- `docs/hardware-components.md` — опис компонентів
- `docs/PRD.md` — Product Requirements Document