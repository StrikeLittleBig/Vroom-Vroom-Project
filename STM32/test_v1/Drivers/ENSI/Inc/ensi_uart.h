/**
 * ENSICAEN
 * 6 Boulevard Maréchal Juin
 * F-14050 Caen Cedex
 *
 * This file is owned by ENSICAEN students. No portion of this
 * document may be reproduced, copied or revised without written
 * permission of the authors.
 */

/**
 * @author  Dimitri Boudier <dimitri.boudier@ensicaen.fr>
 * @version 1.2.0 - 13-02-2024
 *
 * @todo	Nothing.
 * @bug		None.
 * @note 	Select your NUCLEO board by uncommenting its #define below
 * @note 	Only supports NUCLEOS -L073RZ, -L476RG and -F411RE for now.
 */

/**
 * @file ensi_uart.h
 * @brief Custom driver for the USART2 peripheral
 *
 * Provides driver functions for UART communication through the USART2
 * peripheral, which is connected to the STLink Virtual Comm Port.
 */


/* Define to prevent recursive inclusion ------------------------------------*/
#ifndef __ENSI_UART_H
#define __ENSI_UART_H


/* Includes -----------------------------------------------------------------*/

//#define USE_NUCLEO_L073RZ             //!< Select here your NUCLEO board
#define USE_NUCLEO_L476RG             //
//#define USE_NUCLEO_F411RE             //

#if defined(USE_NUCLEO_F411RE)
#include "stm32f4xx_hal.h"

#elif defined(USE_NUCLEO_L073RZ)
#include "stm32l0xx_hal.h"

#elif defined(USE_NUCLEO_L476RG)
#include "stm32l4xx_hal.h"

#endif


/* Exported constants -------------------------------------------------------*/
#define UART_BUFFERSIZE 	256


/* Exported functions -------------------------------------------------------*/

/**
 * USART2 peripheral initialisation function
 */
void ENSI_UART_Init();


/**
 * Sends an 8-bit payload through UART by polling (blocking function)
 *
 * @param[in]   txPayload, the 8-bit data to be sent
 */
void ENSI_UART_PutChar(const uint8_t txPayload);


/**
 * Sends a character string through UART by polling (blocking function)
 *
 * @param[in]   txPayload, pointer to the string to be sent
 *
 * Sends the characters one after the other, until a '\0' is encountered.
 * Because it sends one character at a time, with no peripheral access control,
 * the string transmission can be interrupted by another transmision.
 */
void ENSI_UART_PutString(const uint8_t *txString);


/**
 * Waits for an 8-bit data to be received (blocking function)
 *
 * @param[out]  rxPayload, pointer to the variable into which the received payload will be placed
 *
 * Waits for a data to be placed in the circular buffer (FIFO), then stores
 * it into the variable pointer.
 * The circular buffer is fed by the UART Rx ISR.
 */
void ENSI_UART_GetChar(uint8_t *rxPayload);


/**
 * Retrieves string from the Rx circular buffer (blocking function).
 *
 * @param[out]  rxString, pointer to the string into which the received payloads will be placed
 *
 * Waits for characters to arrive, one after the other, until a '\r' is received.
 * Because it reads one character at a time, with no peripheral access control,
 * the string reception can be interrupted by another reception.
 */
void ENSI_UART_GetString(uint8_t *rxString);


#endif /* __ENSI_UART_H */
