#ifndef I2C_H
// Garde d'inclusion : empeche ce fichier d'etre inclus 2 fois dans la
// meme compilation (evite les erreurs de redefinition).
#define I2C_H

#include "stm32f1xx.h"
// Donne acces aux structures I2C1->, GPIOB->, RCC->, AFIO-> et a tous
// les noms de bits associes (I2C_CR1_PE, AFIO_MAPR_I2C1_REMAP, etc.).

#include <stdint.h>
// Pour les types entiers de taille fixe (uint8_t, uint32_t...).

/**
 * @brief Initialise I2C1 en mode maitre, 100 kHz (Standard Mode).
 *        Broches utilisees : PB8 = SCL, PB9 = SDA (via le remap AFIO,
 *        car sur la Nucleo-F103RB, les pins labellisees SCL/SDA du
 *        connecteur Arduino (D15/D14) sont cablees en dur vers PB8/PB9,
 *        pas vers PB6/PB7 qui sont les broches "par defaut" de l'I2C1).
 *        Suppose une horloge APB1 a 8 MHz (HSI par defaut).
 */
void I2C1_Init(void);

/**
 * @brief Ecrit un octet dans un registre d'un peripherique I2C.
 * @param devAddr Adresse 7 bits du peripherique (ex: 0x68 pour le DS1307)
 * @param reg     Numero du registre interne a ecrire
 * @param data    Octet a ecrire
 */
void I2C_WriteReg(uint8_t devAddr, uint8_t reg, uint8_t data);

/**
 * @brief Lit un octet depuis un registre d'un peripherique I2C.
 */
uint8_t I2C_ReadReg(uint8_t devAddr, uint8_t reg);

/**
 * @brief Ecrit plusieurs octets consecutifs a partir d'un registre de depart.
 *        Utile pour le DS1307 (ecrire les 7 registres heure/date en une fois).
 */
void I2C_WriteBytes(uint8_t devAddr, uint8_t reg, uint8_t *data, uint8_t len);

/**
 * @brief Lit plusieurs octets consecutifs a partir d'un registre de depart.
 */
void I2C_ReadBytes(uint8_t devAddr, uint8_t reg, uint8_t *buffer, uint8_t len);

/**
 * @brief Ecrit un seul octet "brut", sans notion de registre.
 *        Utilise pour le module LCD I2C (PCF8574), qui n'a pas de
 *        registres internes : on lui envoie juste un octet a chaque fois.
 */
void I2C_WriteByteRaw(uint8_t devAddr, uint8_t data);

#endif // I2C_H
