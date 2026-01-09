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
 * @note 	Select your NUCLEO board in the header file
 * @note 	Only supports NUCLEOS -L073RZ, -L476RG and -F411RE for now.
 */

/**
 * @file ensi_uart.c
 * @brief Custom driver for the USART2 peripheral
 *
 * Provides driver functions for UART communication through the USART2
 * peripheral, which is connected to the STLink Virtual Comm Port.
 */

/* Includes -----------------------------------------------------------------*/
#include "ensi_uart.h"


/* Private define -----------------------------------------------------------*/
#define UART_INSTANCE     		USART2
#define UART_PORT				GPIOA
#define UART_TX_PIN				GPIO_PIN_2
#define UART_RX_PIN				GPIO_PIN_3
#define UART_GPIO_SPEED			GPIO_SPEED_FREQ_VERY_HIGH
#define UART_IRQn				USART2_IRQn
#define __UART_CLK_ENABLE()		__USART2_CLK_ENABLE()
#define __UART_CLK_DISABLE()	__USART2_CLK_DISABLE()

#if defined(USE_NUCLEO_F411RE)                      // See STM32F411xx Ref Manual (RM0383) and STM32F411xx datasheet (DS10314)
#define GPIO_AF_UART			GPIO_AF7_USART2         // Datasheet, Table 9. Alternate function mapping
#define	STATUS_REG				SR                      // RefManual, SR = Status Register
#define TX_DATA_REG				DR                      // RefManual, DR = Data Register
#define	RX_DATA_REG				DR                      // RefManual, DR = Data Register
#define	TRANSMIT_COMPLETE_BIT	USART_SR_TC             // RefManual, SR Register, bit 6 = TC = Transmission Complete
#define RX_NEMTPY_BIT			USART_SR_RXNE           // RefManual, SR Register, bit 5 = RXNE = Read data register not empty

#elif defined(USE_NUCLEO_L073RZ)                    // See STM32L0x3 Ref Manual (RM0367) and STM32L073xx Datasheet (DS10685)
#define GPIO_AF_UART			GPIO_AF4_USART2         // Datasheet, Table 17. Alternate function AF0 to AF7
#define	STATUS_REG				ISR                     // RefManual, ISR = Interrupt and Status Register
#define TX_DATA_REG				TDR                     // RefManual, TDR = Transmit Data Register
#define	RX_DATA_REG				RDR                     // RefManual, RDR = Receive Data Register
#define	TRANSMIT_COMPLETE_BIT	USART_ISR_TC            // RefManual, ISR register, bit 6 = TC = Transmission Complete
#define RX_NEMTPY_BIT			USART_ISR_RXNE          // RefManual, ISR Register, bit 5 = RXNE = Read data register not empty

#elif defined(USE_NUCLEO_L476RG)                    // See SMT32L47xx Ref Manual (rm0351) and STM32L476xx Datasheet (DS10198)
#define GPIO_AF_UART			GPIO_AF7_USART2         // Datasheet, Table 17. Alternate function AF0 to AF7
#define	STATUS_REG				ISR                     // RefManual, ISR = Interrupt and Status Register
#define TX_DATA_REG				TDR                     // RefManual, TDR = Transmit Data Register
#define	RX_DATA_REG				RDR                     // RefManual, RDR = Receive Data Register
#define	TRANSMIT_COMPLETE_BIT	USART_ISR_TC            // RefManual, ISR register, bit 6 = TC = Transmission Complete
#define RX_NEMTPY_BIT			USART_ISR_RXNE          // RefManual, ISR Register, bit 5 = RXNE = Read data register not empty
#endif

/* Private macro ------------------------------------------------------------*/
#define UART_DIV_SAMPLING16(_PCLK_, _BAUD_)            ((uint32_t)((((uint64_t)(_PCLK_))*25U)/(4U*((uint64_t)(_BAUD_)))))
#define UART_DIVMANT_SAMPLING16(_PCLK_, _BAUD_)        (UART_DIV_SAMPLING16((_PCLK_), (_BAUD_))/100U)
#define UART_DIVFRAQ_SAMPLING16(_PCLK_, _BAUD_)        ((((UART_DIV_SAMPLING16((_PCLK_), (_BAUD_)) - (UART_DIVMANT_SAMPLING16((_PCLK_), (_BAUD_)) * 100U)) * 16U)\
                                                         + 50U) / 100U)
/* UART BRR = mantissa + overflow + fraction
            = (UART DIVMANT << 4) + (UART DIVFRAQ & 0xF0) + (UART DIVFRAQ & 0x0FU) */
#define UART_BRR_SAMPLING16(_PCLK_, _BAUD_)            ((UART_DIVMANT_SAMPLING16((_PCLK_), (_BAUD_)) << 4U) + \
                                                        (UART_DIVFRAQ_SAMPLING16((_PCLK_), (_BAUD_)) & 0xF0U) + \
                                                        (UART_DIVFRAQ_SAMPLING16((_PCLK_), (_BAUD_)) & 0x0FU))

#define ECHO_ENABLED
#define UART_XON			    0x11
#define UART_XOFF			    0x13


/* Private typedef ----------------------------------------------------------*/
// Type dedicated to circular buffer management
typedef struct {
	volatile uint8_t  buffer[UART_BUFFERSIZE];
	volatile uint32_t writeIndex;
	volatile uint32_t readIndex;
	volatile uint32_t numberOfElements;
	volatile uint8_t  rx_ready;
} ENSI_UART_circularBuffer_t;


/* Private variables --------------------------------------------------------*/
static ENSI_UART_circularBuffer_t uartRxCircularBuffer;


/* Private function prototypes ----------------------------------------------*/
static void ENSI_UART_IRQHandler(void);


/* Public functions ---------------------------------------------------------*/

void ENSI_UART_Init() {
	GPIO_InitTypeDef GPIO_InitStruct;
	uint32_t tmpreg = 0x00;

	/* Peripheral clock enable */
	__UART_CLK_ENABLE();

	/* UART GPIO Configuration */
	GPIO_InitStruct.Pin 	  	= UART_TX_PIN | UART_RX_PIN;
	GPIO_InitStruct.Mode 	  	= GPIO_MODE_AF_PP;
	GPIO_InitStruct.Pull 	  	= GPIO_PULLUP;
	GPIO_InitStruct.Speed 	  	= UART_GPIO_SPEED;
	GPIO_InitStruct.Alternate 	= GPIO_AF_UART;
	HAL_GPIO_Init(UART_PORT, &GPIO_InitStruct);

	/* System interrupt init*/
	HAL_NVIC_SetPriority(UART_IRQn, 10, 10);    //!< @note: USART2 must be of lower-priority (= higher value) than FreeRTOS
	                                            // in order to use "FromISR()" API functions.
	                                            // see https://www.freertos.org/a00110.html#kernel_priority
	HAL_NVIC_EnableIRQ(UART_IRQn);

	/* Disable the peripheral */
	UART_INSTANCE->CR1 &=  ~USART_CR1_UE;

	/*------- UART-associated USART registers setting : CR2 Configuration ------*/
	/* Configure the UART Stop Bits: Set STOP[13:12] bits according
	 * to huart->Init.StopBits value */
	MODIFY_REG(UART_INSTANCE->CR2, (0x3U << 12U), (0x00000000U));

	/*------- UART-associated USART registers setting : CR1 Configuration ------*/
	/* Configure the UART Word Length, Parity and mode: */
	tmpreg = (uint32_t)((0x1U << 3U) | (0x1U << 2U));
	MODIFY_REG(UART_INSTANCE->CR1,
						 (uint32_t)((0x1U << 12) | (0x1U << 10) | (0x1U << 9) | (0x1U << 3) | (0x1U << 2) | (0x1U << 15)),
						 tmpreg);

	/*------- UART-associated USART registers setting : CR3 Configuration ------*/
	/* Configure the UART HFC: Set CTSE and RTSE bits */
	MODIFY_REG(UART_INSTANCE->CR3, ((0x1U << 8) | (0x1U << 9)), 0x00000000U);

	/*---Configure Baudrate BRR register---*/
	UART_INSTANCE->BRR = UART_BRR_SAMPLING16(HAL_RCC_GetPCLK1Freq(), 9600);

	/* In asynchronous mode, the following bits must be kept cleared:
	     - LINEN and CLKEN bits in the USART_CR2 register,
	     - SCEN, HDSEL and IREN  bits in the USART_CR3 register.*/
	CLEAR_BIT(UART_INSTANCE->CR2, (USART_CR2_LINEN | USART_CR2_CLKEN));
	CLEAR_BIT(UART_INSTANCE->CR3, (USART_CR3_SCEN | USART_CR3_HDSEL | USART_CR3_IREN));

	/* Enable the peripheral */
	UART_INSTANCE->CR1 |=  USART_CR1_UE;
  
	/* Initialize RX circular buffer and flag */
	uartRxCircularBuffer.numberOfElements = 0;
	uartRxCircularBuffer.readIndex 	= 0;
	uartRxCircularBuffer.writeIndex = 0;
	uartRxCircularBuffer.rx_ready   = 1;
		
	/* Enable the UART Data Register not empty Interrupt */
	UART_INSTANCE->CR1 |= USART_CR1_RXNEIE;
}



void ENSI_UART_PutChar(const uint8_t txPayload) {
	while( (UART_INSTANCE->STATUS_REG & TRANSMIT_COMPLETE_BIT) == 0 );
	UART_INSTANCE->TX_DATA_REG = txPayload;
}



void ENSI_UART_PutString(const uint8_t *txString) {
	while( *txString != '\0' ) {
		ENSI_UART_PutChar( *txString++ );
	}
}



void ENSI_UART_GetChar(uint8_t *rxPayload) {

	// wait until data arrives into the circular buffer
	while( uartRxCircularBuffer.numberOfElements == 0 );

	*rxPayload = uartRxCircularBuffer.buffer[uartRxCircularBuffer.readIndex];

	// circular buffer processing
	uartRxCircularBuffer.numberOfElements--;
	uartRxCircularBuffer.readIndex++;
	if ( uartRxCircularBuffer.readIndex >= UART_BUFFERSIZE) {
		uartRxCircularBuffer.readIndex = 0;
	}
	
	#ifdef ECHO_ENABLED
	ENSI_UART_PutChar(*rxPayload);
	#endif
	
//	// Software flow control
//    if( (uartRxCircularBuffer.numberOfElements == (UART_BUFFERSIZE >> 1)) && (!uartRxCircularBuffer.rx_ready) ){
//    	ENSI_UART_PutChar(UART_XON);
//    	uartRxCircularBuffer.rx_ready = 1;
//    }
}



void ENSI_UART_GetString(uint8_t *rxString){
	uint8_t tmp;
	uint32_t i = 0;
	
	do{
		ENSI_UART_GetChar(&tmp);
		rxString[i++] = tmp;
	}while( tmp != '\r');
	
	rxString[i-1] = '\0';		// end string with '\0'
}



/* Private functions --------------------------------------------------------*/

/**
 * @brief ISR function according to prototype defined in stm32lxx_startup.s
 */
void USART2_IRQHandler(void) {
	ENSI_UART_IRQHandler();
}


/**
 * @brief ISR dedicated to manage the UART Rx IRQ
 * @param none
 */
static void ENSI_UART_IRQHandler(void) {
	uint8_t tmp;

	// if data available on UART input buffer
	if( (UART_INSTANCE->STATUS_REG & RX_NEMTPY_BIT) != 0 ) 	{
		// retrieve received data
		tmp = (uint8_t) UART_INSTANCE->RX_DATA_REG;

		// save data in circular buffer if is not full
		if( uartRxCircularBuffer.numberOfElements < UART_BUFFERSIZE ) {

			uartRxCircularBuffer.buffer[uartRxCircularBuffer.writeIndex] = tmp;

			// circular buffer processing
			uartRxCircularBuffer.numberOfElements++;
			uartRxCircularBuffer.writeIndex++;
			if( uartRxCircularBuffer.writeIndex >= UART_BUFFERSIZE ) {
				uartRxCircularBuffer.writeIndex = 0;
			}

//			// Software flow control
//			if( uartRxCircularBuffer.numberOfElements == (UART_BUFFERSIZE - 20) ) {
//				UART_INSTANCE->TX_DATA_REG = UART_XOFF;
//				uartRxCircularBuffer.rx_ready = 0;
//			}
		}
		else {
			// no flow control (hard/soft)
		}
	}
}

