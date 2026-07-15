#include "ds1307.h"
#include "i2c.h"
// Depend uniquement du driver I2C generique -> ds1307.c ne touche
// JAMAIS directement aux registres I2C1->, il passe par I2C_WriteBytes
// et I2C_ReadBytes.

static uint8_t dec2bcd(uint8_t valeur)
{
    return ((valeur / 10) << 4) | (valeur % 10);
    // Le DS1307 stocke ses valeurs en BCD : chaque CHIFFRE decimal est
    // code separement sur 4 bits.
    // valeur/10    : extrait les DIZAINES (division entiere)
    // <<4          : decale ce chiffre vers les 4 bits de POIDS FORT
    // valeur%10    : extrait les UNITES (reste de la division)
    // |            : combine les deux zones (dizaines a gauche, unites
    //                a droite) sans qu'elles se marchent dessus.
    // Exemple : 45 -> dizaines=4 (decale: 0100 0000), unites=5 (0000 0101)
    //           OR -> 0100 0101 = 0x45 (represente bien "45" en BCD).
}

static uint8_t bcd2dec(uint8_t valeur)
{
    return ((valeur >> 4) * 10) + (valeur & 0x0F);
    // Operation inverse :
    // valeur>>4 : recupere les 4 bits hauts (dizaines encodees)
    // *10       : leur redonne leur poids reel
    // valeur&0x0F : recupere les 4 bits bas (unites), masque le reste
    // + : additionne les deux pour reconstruire le nombre decimal.
}

void DS1307_SetTime(RTC_Time *t)
{
    uint8_t buffer[7];
    // Tableau temporaire : va contenir les 7 octets a envoyer d'un coup
    // au DS1307.

    buffer[0] = dec2bcd(t->secondes) & 0x7F;
    // t->secondes : accede au champ "secondes" de la structure pointee
    //               par "t" (fleche -> car t est un pointeur).
    // dec2bcd(...) : convertit en BCD.
    // & 0x7F : force le BIT 7 (CH = Clock Halt) a 0 -> CH=0 signifie
    //          "horloge active, ne pas la stopper". 0x7F = 0111 1111,
    //          le AND force le bit de poids fort a 0 en gardant le reste.

    buffer[1] = dec2bcd(t->minutes);
    // Pas de bit special a gerer pour les minutes.

    buffer[2] = dec2bcd(t->heures) & 0x3F;
    // & 0x3F = 0011 1111 : force le bit 6 a 0 (choix du format 12h/24h)
    // -> impose le format 24 HEURES en permanence, plus simple a gerer
    // pour l'affichage (pas de AM/PM a traiter).

    buffer[3] = dec2bcd(t->jourSemaine);
    buffer[4] = dec2bcd(t->jourMois);
    buffer[5] = dec2bcd(t->mois);
    buffer[6] = dec2bcd(t->annee);
    // Pas de bit special pour ces champs, conversion BCD directe.

    I2C_WriteBytes(DS1307_ADDR, 0x00, buffer, 7);
    // Envoie les 7 octets d'un coup, a partir du registre 0x00
    // (secondes) -> le DS1307 les place automatiquement dans l'ordre
    // dans ses 7 registres consecutifs (0x00 a 0x06).
}

void DS1307_GetTime(RTC_Time *t)
{
    uint8_t buffer[7];
    // Va recevoir les 7 octets bruts lus (encore en BCD a ce stade).

    I2C_ReadBytes(DS1307_ADDR, 0x00, buffer, 7);
    // Lit les 7 registres d'un coup, a partir du registre 0x00.

    t->secondes    = bcd2dec(buffer[0] & 0x7F);
    // & 0x7F : masque le bit CH avant conversion (ne doit pas fausser
    // le calcul des dizaines/unites).

    t->minutes     = bcd2dec(buffer[1]);
    t->heures      = bcd2dec(buffer[2] & 0x3F);
    // & 0x3F : masque le bit de format 12h/24h avant conversion.

    t->jourSemaine = bcd2dec(buffer[3]);
    t->jourMois    = bcd2dec(buffer[4]);
    t->mois        = bcd2dec(buffer[5]);
    t->annee       = bcd2dec(buffer[6]);
}
