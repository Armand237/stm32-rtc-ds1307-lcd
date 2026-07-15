# STM32F103 - Horloge RTC DS1307 + LCD I2C

Projet bare-metal sur Nucleo-F103RB : lecture/reglage de l'heure via
un module DS1307, affichage sur ecran LCD 16x2 (module I2C PCF8574).

## Cablage

| Signal | Broche |
|---|---|
| SCL | D15 |
| SDA | D14 |
| VCC | 3.3V |
| GND | GND |

![Montage](docs/photo_montage.jpg)

![Affichage](docs/photo_lcd.jpg)

## Structure

```
Inc/        -> i2c.h, ds1307.h, lcd_i2c.h
Src/        -> i2c.c, ds1307.c, lcd_i2c.c, main.c
```

## Note technique

L'I2C1 est remape sur PB8/PB9 (au lieu de PB6/PB7 par defaut), car
c'est ainsi que la Nucleo-F103RB cable ses broches SCL/SDA (D15/D14).

## Auteur

Gabriel Gakam
