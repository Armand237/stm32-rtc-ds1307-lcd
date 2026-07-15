#include "i2c.h"
#include "ds1307.h"
#include "lcd_i2c.h"
// Chaque module apporte sa propre interface : main.c n'a jamais besoin
// de connaitre les details bas niveau (registres, sequences BCD,
// impulsions ENABLE...).

#define REGLER_HEURE_AU_DEMARRAGE   0
// 1 = ecrit l'heure de depart dans le DS1307 a chaque demarrage.
// 0 = ne touche pas a l'heure deja stockee.
// A mettre a 1 UNE SEULE FOIS pour regler l'heure initiale, puis
// repasser a 0 et reflasher -> le DS1307 garde l'heure en memoire
// grace a sa pile CR2032, meme carte STM32 eteinte.

static void delay_ms(uint32_t ms)
{
    for (volatile uint32_t i = 0; i < ms * 4000; i++);
    // Boucle d'attente logicielle, approximative (facteur ajuste
    // empiriquement pour approcher 1ms/unite a 8MHz HSI).
}

static void FormatDeuxChiffres(char *buffer, uint8_t valeur)
{
    buffer[0] = (char)('0' + (valeur / 10));
    // valeur/10 : extrait le chiffre des DIZAINES.
    // '0' + ... : convertit ce chiffre (0-9) en son caractere ASCII
    //             correspondant, en ajoutant sa valeur au code de '0'.

    buffer[1] = (char)('0' + (valeur % 10));
    // valeur%10 : extrait le chiffre des UNITES (reste de la division).
    // Resultat : ex. 45 devient les 2 caracteres '4' et '5'.
}

int main(void)
{
    I2C1_Init();
    // Configure le bus I2C1 (horloges, remap, broches PB8/PB9, 100kHz).
    // DOIT etre appele en premier.

    LCD_Init();
    // Sequence de demarrage du LCD (depend du bus I2C deja actif).

#if REGLER_HEURE_AU_DEMARRAGE
    // Directive de PREPROCESSEUR : ce bloc n'est meme pas compile si
    // la constante vaut 0 -> aucun cout dans le binaire final.

    RTC_Time heureInitiale;
    // Declare une variable de type RTC_Time, valeurs pour l'instant
    // indeterminees.

    heureInitiale.secondes    = 0;
    heureInitiale.minutes     = 52;
    heureInitiale.heures      = 21;
    heureInitiale.jourSemaine = 2;
    heureInitiale.jourMois    = 15;
    heureInitiale.mois        = 7;
    heureInitiale.annee       = 26;
    // Remplit chaque champ avec la date/heure de depart souhaitee.
    // A adapter a la date/heure REELLE au moment ou tu flashes.

    DS1307_SetTime(&heureInitiale);
    // &heureInitiale : recupere l'ADRESSE de la variable (pas sa
    // valeur), car DS1307_SetTime attend un POINTEUR.
#endif

    LCD_Print("RTC DS1307 pret");
    // Message de confirmation temporaire, pour verifier que le LCD
    // fonctionne avant meme de lire l'heure.

    delay_ms(1000);
    // Laisse le message affiche 1 seconde.

    LCD_Clear();
    // Efface l'ecran avant de commencer l'affichage en boucle.

    while (1)
    // Boucle infinie : un programme embarque ne "termine" jamais.
    {
        RTC_Time maintenant;
        // Structure locale, recoit l'heure ACTUELLE a chaque tour.

        DS1307_GetTime(&maintenant);
        // Remplit "maintenant" avec les valeurs lues via I2C.

        char ligneDate[17];
        char ligneHeure[17];
        // 17 = 16 caracteres utiles (max sur une ligne d'un ecran
        // 16x2) + 1 pour le '\0' final obligatoire.

        char jour[3], mois[3], annee[3];
        char heure[3], minute[3], seconde[3];
        // Petits buffers temporaires (2 chiffres + '\0').

        FormatDeuxChiffres(jour,    maintenant.jourMois);
        FormatDeuxChiffres(mois,    maintenant.mois);
        FormatDeuxChiffres(annee,   maintenant.annee);
        FormatDeuxChiffres(heure,   maintenant.heures);
        FormatDeuxChiffres(minute,  maintenant.minutes);
        FormatDeuxChiffres(seconde, maintenant.secondes);
        // Convertit chaque valeur numerique en 2 caracteres ASCII.

        ligneDate[0]=jour[0];    ligneDate[1]=jour[1];    ligneDate[2]='/';
        ligneDate[3]=mois[0];    ligneDate[4]=mois[1];    ligneDate[5]='/';
        ligneDate[6]='2';        ligneDate[7]='0';        ligneDate[8]=annee[0];
        ligneDate[9]=annee[1];   ligneDate[10]='\0';
        // Construction manuelle de "JJ/MM/AAAA" (pas de sprintf en bare
        // metal pur). "20" fige en dur (annees 2000-2099 supposees).
        // Le '\0' final est INDISPENSABLE, sinon LCD_Print lirait la
        // memoire indefiniment au-dela du tableau.

        ligneHeure[0]=heure[0];   ligneHeure[1]=heure[1];   ligneHeure[2]=':';
        ligneHeure[3]=minute[0];  ligneHeure[4]=minute[1];  ligneHeure[5]=':';
        ligneHeure[6]=seconde[0]; ligneHeure[7]=seconde[1]; ligneHeure[8]='\0';
        // Meme principe pour "HH:MM:SS".

        LCD_SetCursor(0, 0);
        // Positionne le curseur en haut a gauche.

        LCD_Print(ligneDate);
        // Affiche la date sur la 1ere ligne.

        LCD_SetCursor(1, 0);
        // Positionne le curseur au debut de la 2e ligne.

        LCD_Print(ligneHeure);
        // Affiche l'heure sur la 2e ligne.

        delay_ms(500);
        // Attend 500ms avant de relire et rafraichir l'affichage.
    }
}
