#ifndef LCD_I2C_H
#define LCD_I2C_H

#include "stm32f1xx.h"
#include <stdint.h>

#define LCD_I2C_ADDR   0x027
// Adresse I2C 7 bits du module PCF8574 relie au LCD. Valeur la plus
// courante : 0x27. Si l'ecran reste vide malgre un cablage correct,
// essaie 0x3F a la place (depend du fabricant du module).

void LCD_Init(void);
// Sequence de demarrage obligatoire avant tout affichage.

void LCD_Clear(void);
// Efface tout et replace le curseur en position (0,0).

void LCD_SetCursor(uint8_t ligne, uint8_t colonne);
// Deplace le curseur avant le prochain texte affiche.
// ligne : 0 ou 1 (ecran 16x2 -> seulement 2 lignes).
// colonne : 0 a 15.

void LCD_Print(const char *str);
// "const char *str" = pointeur vers une chaine en lecture seule.

#endif // LCD_I2C_H
