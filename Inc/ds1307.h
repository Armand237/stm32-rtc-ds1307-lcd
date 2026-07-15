#ifndef DS1307_H
#define DS1307_H

#include "stm32f1xx.h"
#include <stdint.h>

#define DS1307_ADDR   0x68
// Adresse I2C 7 bits du DS1307, fixee par le fabricant (contrairement
// au module LCD dont l'adresse peut varier selon le fabricant du PCF8574).

typedef struct
{
    uint8_t secondes;
    uint8_t minutes;
    uint8_t heures;
    uint8_t jourSemaine;
    uint8_t jourMois;
    uint8_t mois;
    uint8_t annee;
} RTC_Time;
// Cree un nouveau type "RTC_Time", regroupant 7 valeurs representant
// une date/heure complete. Toutes en DECIMAL NORMAL ici (pas en BCD) -
// la conversion est geree en interne dans ds1307.c.

void DS1307_SetTime(RTC_Time *t);
// Prend un POINTEUR vers une structure (pas une copie), pour eviter de
// dupliquer inutilement 7 octets a chaque appel.

void DS1307_GetTime(RTC_Time *t);
// Remplit la structure pointee par "t" avec l'heure lue.

#endif // DS1307_H
