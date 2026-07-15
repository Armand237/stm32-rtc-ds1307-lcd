#include "lcd_i2c.h"
#include "i2c.h"

#define LCD_BACKLIGHT   0x08
// bit 3 de l'octet envoye au PCF8574 : controle le retro-eclairage.
// 0x08 = 0000 1000, seul le bit 3 est a 1.

#define LCD_ENABLE      0x04
// bit 2 : signal "Enable". Une IMPULSION (1 puis 0) dit au LCD
// "lis maintenant les 4 bits de donnees que je viens de placer".

#define LCD_RS_DATA     0x01
// bit 0 : Register Select. 1 = "ce que j'envoie est une DONNEE a
// afficher" (un caractere).

#define LCD_RS_COMMAND  0x00
// 0 = "ce que j'envoie est une COMMANDE" (effacer, positionner le
// curseur, configurer le mode...), pas un caractere.

static void delay_us(uint32_t us)
{
    for (volatile uint32_t i = 0; i < us * 8; i++);
    // Boucle d'attente logicielle "vide", approximative mais suffisante
    // pour respecter les delais minimaux du controleur LCD HD44780.
}

static void LCD_SendNibble(uint8_t nibble, uint8_t rs)
{
    uint8_t data = (nibble & 0xF0) | LCD_BACKLIGHT | rs;
    // nibble & 0xF0 : garde uniquement les 4 bits hauts (D4-D7 du LCD
    //                 correspondent aux bits 4-7 de cet octet).
    // | LCD_BACKLIGHT : ajoute le bit retro-eclairage (toujours allume).
    // | rs : ajoute le bit Register Select (donnee ou commande).

    I2C_WriteByteRaw(LCD_I2C_ADDR, data | LCD_ENABLE);
    // Envoie l'octet avec ENABLE=1 -> prepare le LCD a lire les 4 bits.

    delay_us(1);
    // Court delai pour stabiliser le signal avant de redescendre ENABLE.

    I2C_WriteByteRaw(LCD_I2C_ADDR, data & ~LCD_ENABLE);
    // Renvoie le meme octet mais ENABLE=0 -> ce FRONT DESCENDANT
    // declenche la lecture reelle par le controleur HD44780.

    delay_us(50);
    // Delai apres l'impulsion, laisse au LCD le temps de traiter.
}

static void LCD_SendByte(uint8_t valeur, uint8_t rs)
{
    LCD_SendNibble(valeur & 0xF0, rs);
    // Envoie d'abord les 4 bits de POIDS FORT.

    LCD_SendNibble((valeur << 4) & 0xF0, rs);
    // (valeur<<4) : decale les 4 bits de poids FAIBLE vers la position
    //               haute, pour qu'ils soient lus a leur tour par
    //               LCD_SendNibble. Complete l'envoi de l'octet entier
    //               en mode "4 bits" (protocole standard HD44780 quand
    //               seulement 4 fils de donnees sont cables, via le PCF8574).
}

static void LCD_SendCommand(uint8_t cmd)
{
    LCD_SendByte(cmd, LCD_RS_COMMAND);
}

static void LCD_SendData(uint8_t data)
{
    LCD_SendByte(data, LCD_RS_DATA);
}

void LCD_Init(void)
{
    delay_us(50000);
    // Attente initiale (50ms) apres mise sous tension : le LCD a besoin
    // de temps pour etre pret (minimum ~40ms selon le datasheet HD44780).

    LCD_SendNibble(0x30, LCD_RS_COMMAND);
    delay_us(5000);
    LCD_SendNibble(0x30, LCD_RS_COMMAND);
    delay_us(200);
    LCD_SendNibble(0x30, LCD_RS_COMMAND);
    delay_us(200);
    // Ces 3 envois de 0x30 forcent le LCD dans un etat CONNU (mode 8
    // bits), sequence "magique" imposee par le datasheet HD44780.

    LCD_SendNibble(0x20, LCD_RS_COMMAND);
    delay_us(200);
    // Bascule reellement en MODE 4 BITS (celui utilise pour la suite).

    LCD_SendCommand(0x28);
    // Function Set : 4 bits (DL=0), 2 lignes (N=1), police 5x8 (F=0).

    LCD_SendCommand(0x0C);
    // Display Control : ecran allume (D=1), curseur invisible (C=0),
    // pas de clignotement (B=0).

    LCD_SendCommand(0x06);
    // Entry Mode Set : le curseur avance automatiquement a droite
    // apres chaque caractere (I/D=1), pas de decalage global (S=0).

    LCD_SendCommand(0x01);
    // Clear Display : efface tout, replace le curseur en (0,0).

    delay_us(2000);
    // Le Clear est plus lent que les autres commandes, on lui laisse
    // plus de temps pour s'executer completement.
}

void LCD_Clear(void)
{
    LCD_SendCommand(0x01);
    delay_us(2000);
    // Reutilisable a tout moment en cours de fonctionnement.
}

void LCD_SetCursor(uint8_t ligne, uint8_t colonne)
{
    uint8_t adresse = colonne + (ligne == 0 ? 0x00 : 0x40);
    // Le HD44780 stocke chaque ligne a une adresse DIFFERENTE et NON
    // CONTIGUE : ligne 0 commence a 0x00, ligne 1 commence a 0x40
    // (saut impose par le materiel, meme sur un ecran 16x2).
    // Operateur ternaire : "si ligne==0, utilise 0x00, sinon 0x40".

    LCD_SendCommand(0x80 | adresse);
    // 0x80 : commande "Set DDRAM Address" (positionner le curseur).
    // | adresse : combine avec l'adresse calculee.
}

void LCD_Print(const char *str)
{
    while (*str)
    // *str : dereference le pointeur -> lit le caractere pointe.
    // La boucle continue tant que ce caractere n'est PAS '\0'
    // (le caractere nul qui termine toute chaine en C).
    {
        LCD_SendData((uint8_t)(*str));
        // Envoie le caractere courant pour affichage.

        str++;
        // Avance le pointeur -> pointera le caractere SUIVANT au
        // prochain tour de boucle.
    }
}
