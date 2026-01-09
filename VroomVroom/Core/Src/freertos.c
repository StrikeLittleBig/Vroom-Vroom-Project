/* USER CODE BEGIN Header */
	/**
	  ******************************************************************************
	  * File Name          : freertos.c
	  * Description        : Code for freertos applications
	  ******************************************************************************
	  * @attention
	  *
	  * Copyright (c) 2025 STMicroelectronics.
	  * All rights reserved.
	  *
	  * This software is licensed under terms that can be found in the LICENSE file
	  * in the root directory of this software component.
	  * If no LICENSE file comes with this software, it is provided AS-IS.
	  *
	  ******************************************************************************
	  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ensi_uart.h"
#include <string.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */

extern TIM_HandleTypeDef htim1;
extern DMA_HandleTypeDef hdma_tim1_ch3;
extern DMA_HandleTypeDef hdma_tim1_ch2;

static uint16_t led_buffer[LED_BUFFER_SIZE];
static uint8_t RGB_buffer[LED_COUNT][3];

static uint16_t circle_led_buffer[CIRCLE_LED_BUFFER_SIZE];
static uint8_t circle_RGBW_buffer[CIRCLE_LED_COUNT][4];

/* USER CODE END Variables */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

static void byte_to_pwm(uint8_t byte, uint16_t *out);

void np_set_pixel(uint16_t i, uint8_t r, uint8_t g, uint8_t b);
void np_set_all_leds(uint8_t r, uint8_t g, uint8_t b);
void np_clear(void);
void np_show(void);

void np_set_pixel_circle(uint16_t i, uint8_t r, uint8_t g, uint8_t b, uint8_t w);
void np_set_all_leds_circle(uint8_t r, uint8_t g, uint8_t b, uint8_t w);
void np_clear_circle(void);
void np_show_circle(void);

/*
 *
 * STICK LED
 *
 */

static void byte_to_pwm(uint8_t byte, uint16_t *out) {
  for (int i = 0; i < 8; i++) {
    out[i] = (byte & (1 << (7 - i))) ? T1H_TICKS : T0H_TICKS;
  }
}


void np_set_pixel(uint16_t i, uint8_t r, uint8_t g, uint8_t b) {
  if (i >= LED_COUNT) return;
  RGB_buffer[i][0] = g;
  RGB_buffer[i][1] = r;
  RGB_buffer[i][2] = b;
}

void np_set_all_leds(uint8_t r, uint8_t g, uint8_t b){
	for (int pos = 0; pos < LED_COUNT; pos++) {
		np_set_pixel(pos, r, g, b);
	}
}

void np_clear(void) {
  memset(RGB_buffer, 0, sizeof(RGB_buffer));
}


void np_show(void) {
  uint16_t *p = led_buffer;
  for (int i = 0; i < LED_COUNT; i++) {
    byte_to_pwm(RGB_buffer[i][0], p); p += 8; // G
    byte_to_pwm(RGB_buffer[i][1], p); p += 8; // R
    byte_to_pwm(RGB_buffer[i][2], p); p += 8; // B
  }
  for (int i = 0; i < RESET_SLOTS; i++) *p++ = 0;

  __HAL_TIM_SET_AUTORELOAD(&htim1, ARR_VALUE);

  HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_3,
                        (uint32_t*)led_buffer, (uint32_t)(p - led_buffer));

  while (hdma_tim1_ch3.State != HAL_DMA_STATE_READY) { }

  HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_3);

}


/*
 *
 * CIRCLE LED
 *
 */

void np_set_pixel_circle(uint16_t i, uint8_t r, uint8_t g, uint8_t b, uint8_t w) {
  if (i >= CIRCLE_LED_COUNT) return;
  circle_RGBW_buffer[i][0] = g;
  circle_RGBW_buffer[i][1] = r;
  circle_RGBW_buffer[i][2] = b;
  circle_RGBW_buffer[i][3] = w;
}


void np_set_all_leds_circle(uint8_t r, uint8_t g, uint8_t b, uint8_t w){
	for (int pos = 0; pos < CIRCLE_LED_COUNT; pos++) {
		np_set_pixel_circle(pos, r, g, b, w);
	}
}


void np_clear_circle(void) {
  memset(circle_RGBW_buffer, 0, sizeof(circle_RGBW_buffer));
}


void np_show_circle(void) {
  uint16_t *p = circle_led_buffer;
  for (int i = 0; i < CIRCLE_LED_COUNT; i++) {
    byte_to_pwm(circle_RGBW_buffer[i][0], p); p += 8; // G
    byte_to_pwm(circle_RGBW_buffer[i][1], p); p += 8; // R
    byte_to_pwm(circle_RGBW_buffer[i][2], p); p += 8; // B
    byte_to_pwm(circle_RGBW_buffer[i][3], p); p += 8; // W
  }
  for (int i = 0; i < RESET_SLOTS; i++) *p++ = 0;

  __HAL_TIM_SET_AUTORELOAD(&htim1, ARR_VALUE);

  HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_2,
                        (uint32_t*)circle_led_buffer, (uint32_t)(p - circle_led_buffer));

  // Attendre la fin du DMA
  while (hdma_tim1_ch2.State != HAL_DMA_STATE_READY) { }

  HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_2);

}



/* USER CODE END FunctionPrototypes */

/* GetIdleTaskMemory prototype (linked to static allocation support) */
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize );

/* USER CODE BEGIN GET_IDLE_TASK_MEMORY */
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t xIdleStack[configMINIMAL_STACK_SIZE];

void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer, StackType_t **ppxIdleTaskStackBuffer, uint32_t *pulIdleTaskStackSize ) {
	*ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
	*ppxIdleTaskStackBuffer = &xIdleStack[0];
	 *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
	  /* place for user code */
}
/* USER CODE END GET_IDLE_TASK_MEMORY */

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
TickType_t tick;

void app_init(void);

void LED_ON(void *pvParameters);
void CircleLED_ON(void *pvParameters);

void app_init(void){

	xTaskCreate(LED_ON, "Barrette_LED", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);
	xTaskCreate(CircleLED_ON, "LED_Circulaire", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY, NULL);

}

void LED_ON(void *pvParameters){
	while(1){

			np_clear();
			for (int pos = 0; pos < 4; pos++) {
				np_set_pixel(pos, 255, 0, 0);
			}
			for (int pos = 4; pos < LED_COUNT; pos++) {
				np_set_pixel(pos, 0, 0, 255);
			}
	  	    np_show();

	  	    HAL_Delay(200);

			np_clear();
			for (int pos = 0; pos < 4; pos++) {
				np_set_pixel(pos, 0, 0, 255);
			}
			for (int pos = 4; pos < LED_COUNT; pos++) {
				np_set_pixel(pos, 255, 0, 0);
			}
	  	    np_show();

	  	    HAL_Delay(200);
	}
}


void CircleLED_ON(void *pvParameters){

	while(1){

		np_clear_circle();
		np_set_all_leds_circle(0, 0, 255, 0);
  	    np_show_circle();

  	    HAL_Delay(200);

  	    np_clear_circle();
  	    np_set_all_leds_circle(255, 0, 0, 0);
  	    np_show_circle();

  	    HAL_Delay(200);
	}


}

/* USER CODE END Application */
