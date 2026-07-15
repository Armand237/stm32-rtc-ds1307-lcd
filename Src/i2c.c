#include "i2c.h"
// Notre propre interface (prototypes declares dans i2c.h).

/* ==================== FONCTIONS INTERNES (static) ==================== */
// "static" = visibles uniquement dans ce fichier. Le reste du programme
// passe toujours par les fonctions publiques plus bas.

static void I2C1_Start(void)
{
    I2C1->CR1 |= I2C_CR1_START;
    // Genere la "condition START" -> signal special qui annonce a tous
    // les esclaves du bus qu'une nouvelle communication commence.

    while (!(I2C1->SR1 & I2C_SR1_SB));
    // I2C_SR1_SB : "Start Bit", passe a 1 une fois le START reellement
    // envoye electriquement. On attend cette confirmation (polling bloquant).
}

static void I2C1_Stop(void)
{
    I2C1->CR1 |= I2C_CR1_STOP;
    // Genere la "condition STOP" -> libere le bus, termine la transaction.
}

static void I2C1_SendAddress(uint8_t devAddr, uint8_t readMode)
{
    I2C1->DR = (devAddr << 1) | (readMode ? 1 : 0);
    // devAddr << 1 : l'adresse 7 bits est decalee pour occuper les bits
    //                7 a 1 du registre DR.
    // | (readMode?1:0) : le bit 0 indique la direction : 0=ecriture,
    //                1=lecture. Operateur ternaire : "si readMode est
    //                vrai (different de 0), utilise 1, sinon utilise 0".

    while (!(I2C1->SR1 & I2C_SR1_ADDR));
    // I2C_SR1_ADDR : passe a 1 quand l'esclave a reconnu son adresse et
    // a repondu par un accuse de reception (ACK).

    (void)I2C1->SR1;
    (void)I2C1->SR2;
    // Sequence imposee par le materiel : le flag ADDR ne s'efface QUE
    // si on lit SR1 PUIS SR2 dans cet ordre. (void) = on "jette"
    // volontairement ces valeurs lues, juste pour effacer le flag.
}

static void I2C1_WriteByte(uint8_t data)
{
    while (!(I2C1->SR1 & I2C_SR1_TXE));
    // I2C_SR1_TXE : "Transmit Data Register Empty", attend que DR soit
    // libre avant d'y ecrire un nouvel octet.

    I2C1->DR = data;
    // Ecrit l'octet -> declenche son envoi bit par bit sur SDA.

    while (!(I2C1->SR1 & I2C_SR1_BTF));
    // I2C_SR1_BTF : "Byte Transfer Finished", confirme l'envoi complet
    // ET la reception d'un ACK de l'esclave.
}

static uint8_t I2C1_ReadByteAck(void)
{
    I2C1->CR1 |= I2C_CR1_ACK;
    // Active l'envoi automatique d'un ACK apres reception -> dit a
    // l'esclave "envoie-moi la suite, il reste des octets a lire".

    while (!(I2C1->SR1 & I2C_SR1_RXNE));
    // I2C_SR1_RXNE : "Receive Data Register Not Empty", passe a 1 des
    // qu'un octet est recu et disponible dans DR.

    return (uint8_t)I2C1->DR;
    // Retourne l'octet recu (le lire efface aussi RXNE automatiquement).
}

static uint8_t I2C1_ReadByteNack(void)
{
    I2C1->CR1 &= ~I2C_CR1_ACK;
    // Desactive l'ACK -> signale a l'esclave "c'est le dernier octet
    // que je veux lire".

    I2C1->CR1 |= I2C_CR1_STOP;
    // Programme aussi l'envoi du STOP juste apres cette lecture.

    while (!(I2C1->SR1 & I2C_SR1_RXNE));
    return (uint8_t)I2C1->DR;
}

/* ==================== FONCTIONS PUBLIQUES ==================== */

void I2C1_Init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_IOPBEN;
    // Active l'horloge de GPIOB (necessaire pour configurer PB8/PB9).

    RCC->APB2ENR |= RCC_APB2ENR_AFIOEN;
    // Active l'horloge de l'AFIO (Alternate Function I/O) -> c'est ce
    // module qui gere les remaps de peripheriques. On en a besoin pour
    // pouvoir modifier AFIO->MAPR juste apres.

    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
    // Active l'horloge du peripherique I2C1 lui-meme (bus APB1).

    AFIO->MAPR |= AFIO_MAPR_I2C1_REMAP;
    // AFIO->MAPR : registre qui controle tous les remaps de peripheriques
    // du chip (I2C, USART, SPI, etc.).
    // AFIO_MAPR_I2C1_REMAP : bit specifique qui redirige l'I2C1 de ses
    // broches "par defaut" (PB6/PB7) vers PB8/PB9 -> necessaire car
    // c'est ainsi que la Nucleo-F103RB a cable ses pins labellisees
    // SCL/SDA (D15/D14) sur son connecteur Arduino.

    GPIOB->CRH &= ~(0xF << ((8 - 8) * 4));
    GPIOB->CRH |=  (0xF << ((8 - 8) * 4));
    // GPIOB->CRH : registre de config des broches PB8 a PB15 (CRH="High").
    // (8-8)=0    : PB8 est la 1ere broche geree par CRH (indice local 0).
    // 0xF        : MODE=11 (sortie 50MHz) + CNF=11 (Alternate Function
    //              Open-Drain) -> obligatoire en I2C (chaque appareil
    //              peut seulement "tirer vers 0", jamais forcer un "1",
    //              pour eviter les conflits electriques sur le bus partage).

    GPIOB->CRH &= ~(0xF << ((9 - 8) * 4));
    GPIOB->CRH |=  (0xF << ((9 - 8) * 4));
    // (9-8)=1 : PB9 est la 2e broche geree par CRH (indice local 1).
    // Meme configuration que PB8.

    I2C1->CR2   = 8;
    // Le champ FREQ doit indiquer la frequence de l'horloge APB1 EN MHz
    // -> 8, car HSI = 8 MHz par defaut (sans PLL configure).

    I2C1->CCR   = 40;
    // Determine la duree des phases haute/basse de SCL pour atteindre
    // 100 kHz (Standard Mode) : CCR = Fapb1 / (2 * 100000) = 40.

    I2C1->TRISE = 9;
    // Temps de montee maximal autorise sur le bus (1000ns en mode
    // standard) : TRISE = Fapb1_MHz + 1 = 9.

    I2C1->CR1 |= I2C_CR1_PE;
    // "Peripheral Enable" -> active enfin le peripherique I2C1 dans
    // son ensemble. Sans ce bit, rien de configure avant ne prend effet.
}

void I2C_WriteReg(uint8_t devAddr, uint8_t reg, uint8_t data)
{
    I2C1_Start();
    // Envoie la condition START -> commence une nouvelle transaction.

    I2C1_SendAddress(devAddr, 0);
    // Envoie l'adresse esclave + bit "ecriture" (0), attend l'ACK.

    I2C1_WriteByte(reg);
    // Envoie le NUMERO du registre a ecrire.

    I2C1_WriteByte(data);
    // Envoie la VALEUR a y placer.

    I2C1_Stop();
    // Libere le bus.
}

uint8_t I2C_ReadReg(uint8_t devAddr, uint8_t reg)
{
    uint8_t valeur;
    // Stockera la valeur lue, avant de la retourner a l'appelant.

    I2C1_Start();
    I2C1_SendAddress(devAddr, 0);
    I2C1_WriteByte(reg);
    // Premiere phase : on ecrit le numero du registre qu'on VEUT LIRE
    // (il faut d'abord "pointer" dessus avant de pouvoir en lire la valeur).

    I2C1_Start();
    // "Restart" : relance un START sans avoir envoye de STOP avant -
    // garde le bus reserve pour nous entre les deux phases.

    I2C1_SendAddress(devAddr, 1);
    // Deuxieme phase : renvoie l'adresse avec le bit "lecture" (1).

    valeur = I2C1_ReadByteNack();
    // Un seul octet a lire -> pas d'ACK, STOP automatique juste apres.

    return valeur;
}

void I2C_WriteBytes(uint8_t devAddr, uint8_t reg, uint8_t *data, uint8_t len)
{
    I2C1_Start();
    I2C1_SendAddress(devAddr, 0);
    I2C1_WriteByte(reg);
    // Pointe vers le premier registre a partir duquel on va ecrire.

    for (uint8_t i = 0; i < len; i++)
    {
        I2C1_WriteByte(data[i]);
        // Envoie chaque octet du tableau les uns apres les autres.
        // Le DS1307 incremente automatiquement son propre pointeur de
        // registre interne apres chaque octet recu.
    }

    I2C1_Stop();
}

void I2C_ReadBytes(uint8_t devAddr, uint8_t reg, uint8_t *buffer, uint8_t len)
{
    I2C1_Start();
    I2C1_SendAddress(devAddr, 0);
    I2C1_WriteByte(reg);
    // Pointe vers le premier registre a partir duquel lire.

    I2C1_Start();
    I2C1_SendAddress(devAddr, 1);
    // Restart en mode lecture.

    for (uint8_t i = 0; i < len; i++)
    {
        if (i == (len - 1))
        {
            buffer[i] = I2C1_ReadByteNack();
            // Dernier octet : pas d'ACK + STOP automatique.
        }
        else
        {
            buffer[i] = I2C1_ReadByteAck();
            // Tous les autres : ACK pour dire "continue d'envoyer".
        }
    }
}

void I2C_WriteByteRaw(uint8_t devAddr, uint8_t data)
{
    I2C1_Start();
    I2C1_SendAddress(devAddr, 0);
    I2C1_WriteByte(data);
    I2C1_Stop();
    // Version simplifiee sans notion de "registre" : utile pour le
    // PCF8574 (module LCD), qui interprete directement chaque octet
    // recu comme une commande de controle de ses broches de sortie.
}
