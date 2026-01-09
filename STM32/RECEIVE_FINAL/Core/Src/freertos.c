/**
  ******************************************************************************
  * @file    freertos.c
  * @brief   RTOS objects: queues, tasks, MX_FREERTOS_Init()
  ******************************************************************************
  */

#include "cmsis_os.h"
#include "main.h"

/* FreeRTOS */
#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

/* C std */
#include <string.h>
#include <stdio.h>
#include <stdint.h>

/* Extern HAL handles (from main.c) */
extern UART_HandleTypeDef huart2;
extern TIM_HandleTypeDef  htim2;   /* TIM2_CH3 -> PB10 (AF1) */
extern TIM_HandleTypeDef  htim3;   /* TIM3_CH1 -> PB4  (AF2) */

/* Partagé avec l'ISR SPI */
QueueHandle_t spiRxQueue = NULL;

/* Etats (optionnel) */
volatile float LX_value;
volatile float RT_value;
volatile float LT_value;

/* Trame SPI brute */
#define FRAME_LEN  64
typedef struct { uint8_t bytes[FRAME_LEN]; } SpiFrame_t;

/* Protos */
static void StartDefaultTask(void const * argument);
void StartDirTask(void const * argument);
void StartSpdTask(void const * argument);

/* Format décodé (offset 3) */
typedef struct {
  uint32_t buttons;
  float    lx;
  float    ly;
  float    rx;
  float    ry;
  float    lt;
  float    rt;
} GamepadFrame_t;

static inline uint32_t read_u32_le(const uint8_t *p) { uint32_t v; memcpy(&v,p,4); return v; }
static inline float    read_f32_le(const uint8_t *p) { float    f; memcpy(&f,p,4); return f; }

static void decode_gamepad_frame(const uint8_t *data, GamepadFrame_t *out)
{
  size_t off = 3;  /* on saute 3 octets */
  out->buttons = read_u32_le(&data[off]); off += 4;
  out->lx      = read_f32_le(&data[off]); off += 4;
  out->ly      = read_f32_le(&data[off]); off += 4;
  out->rx      = read_f32_le(&data[off]); off += 4;
  out->ry      = read_f32_le(&data[off]); off += 4;
  out->lt      = read_f32_le(&data[off]); off += 4;
  out->rt      = read_f32_le(&data[off]); off += 4;
}

/* Messages des queues */
typedef struct { float lx; } DirectionMsg;
typedef struct { float lt; float rt; } VitesseMsg;

QueueHandle_t qDirection = NULL;
QueueHandle_t qVitesse   = NULL;

/* Bornes et gains */
static const uint16_t DIR_MIN    = 1200;
static const uint16_t DIR_CENTER = 1400;
static const uint16_t DIR_MAX    = 1600;
static const int16_t  DIR_SCALE  = 200;   /* duty_dir = 1400 - 200*lx */

static const uint16_t SPD_CENTER = 1400;  /* test_v1 neutre */
static const uint16_t SPD_MIN    = 1300;
static const uint16_t SPD_MAX    = 1500;
static const int16_t  SPD_SCALE  = 50;    /* duty_spd = 1400 + 50*(rt - lt) */

static inline int clamp_i(int v, int lo, int hi) {
  if (v < lo) return lo; if (v > hi) return hi; return v;
}

static void fmt_f2(char *dst, size_t sz, float v)
{
  int neg = (v < 0.0f);
  float a = neg ? -v : v;
  int32_t ip = (int32_t)a;
  int32_t frac = (int32_t)((a - (float)ip)*100.0f + 0.5f);
  if (frac >= 100) { frac -= 100; ip += 1; }
  snprintf(dst, sz, neg ? "-%ld.%02ld" : "%ld.%02ld", (long)ip, (long)frac);
}

/* Log de config PWM et AF des pins */
static void LogPwmSetupOnce(void)
{
  uint32_t t2_psc = htim2.Init.Prescaler, t2_arr = htim2.Init.Period;
  uint32_t t3_psc = htim3.Init.Prescaler, t3_arr = htim3.Init.Period;
  uint32_t af_pb10 = (GPIOB->AFR[1] >> ((10U-8U)*4U)) & 0xF;  /* PB10 -> AFRH idx 2 */
  uint32_t af_pb4  = (GPIOB->AFR[0] >> (4U*4U)) & 0xF;        /* PB4  -> AFRL idx 4  */

  char buf[196];
  int n = snprintf(buf, sizeof buf,
      "[CFG] TIM2:PSC=%lu ARR=%lu  TIM3:PSC=%lu ARR=%lu\r\n"
      "[CFG] PB10.AF=%lu (exp 1)   PB4.AF=%lu (exp 2)\r\n"
      "[CFG] DIR[%u,%u,%u]  SPD[%u,%u,%u]\r\n",
      (unsigned long)t2_psc, (unsigned long)t2_arr,
      (unsigned long)t3_psc, (unsigned long)t3_arr,
      (unsigned long)af_pb10, (unsigned long)af_pb4,
      DIR_MIN, DIR_CENTER, DIR_MAX, SPD_MIN, SPD_CENTER, SPD_MAX);
  HAL_UART_Transmit(&huart2, (uint8_t*)buf, (uint16_t)n, 100);
}

/* Init FreeRTOS */
void MX_FREERTOS_Init(void)
{
  /* Banniere pour être sûr de l’image flashée */
  {
    char b[96];
    int n = snprintf(b, sizeof b, "[BOOT] FW=%s %s\r\n", __DATE__, __TIME__);
    HAL_UART_Transmit(&huart2, (uint8_t*)b, (uint16_t)n, 50);
  }

  spiRxQueue = xQueueCreate(8, sizeof(SpiFrame_t));
  if (!spiRxQueue) { taskDISABLE_INTERRUPTS(); for(;;){} }

  qDirection = xQueueCreate(1, sizeof(DirectionMsg));
  qVitesse   = xQueueCreate(1, sizeof(VitesseMsg));

  {
    char buf[96];
    int n = snprintf(buf, sizeof buf, "[BOOT] qDir=%p qSpd=%p free=%lu min=%lu\r\n",
                     (void*)qDirection, (void*)qVitesse,
                     (unsigned long)xPortGetFreeHeapSize(),
                     (unsigned long)xPortGetMinimumEverFreeHeapSize());
    HAL_UART_Transmit(&huart2, (uint8_t*)buf, (uint16_t)n, 50);
  }

  osThreadDef(defaultTask, StartDefaultTask, osPriorityNormal, 0, 256);
  osThreadId defH = osThreadCreate(osThread(defaultTask), NULL);

  osThreadDef(dirTask, StartDirTask, osPriorityAboveNormal, 0, 256);
  osThreadId dirH = osThreadCreate(osThread(dirTask), NULL);

  osThreadDef(spdTask, StartSpdTask, osPriorityAboveNormal, 0, 256);
  osThreadId spdH = osThreadCreate(osThread(spdTask), NULL);

  {
    char buf[96];
    int n = snprintf(buf, sizeof buf, "[BOOT] default=%p dir=%p spd=%p\r\n",
                     defH, dirH, spdH);
    HAL_UART_Transmit(&huart2, (uint8_t*)buf, (uint16_t)n, 50);
  }

  LogPwmSetupOnce();
}

/* ==== TASKS ================================================================ */
static void StartDefaultTask(void const * argument)
{
  SpiFrame_t frame;
  GamepadFrame_t g;

  for(;;) {
    if (xQueueReceive(spiRxQueue, &frame, portMAX_DELAY) == pdTRUE) {
      decode_gamepad_frame(frame.bytes, &g);

      LT_value = g.lt;
      RT_value = g.rt;
      LX_value = g.lx;

      DirectionMsg dmsg = { .lx = LX_value };
      VitesseMsg   vmsg = { .lt = LT_value, .rt = RT_value };
      (void)xQueueOverwrite(qDirection, &dmsg);
      (void)xQueueOverwrite(qVitesse,   &vmsg);

      char lx[12], lt[12], rt[12], line[64];
      fmt_f2(lx, sizeof lx, LX_value);
      fmt_f2(lt, sizeof lt, LT_value);
      fmt_f2(rt, sizeof rt, RT_value);
      int n = snprintf(line, sizeof line, "LX=%s  LT=%s  RT=%s\r\n", lx, lt, rt);
      HAL_UART_Transmit(&huart2, (uint8_t*)line, (uint16_t)n, 50);
    }
  }
}

/* Direction : TIM2_CH3 / PB10 */
void StartDirTask(void const * argument)
{
  HAL_UART_Transmit(&huart2, (uint8_t*)"[DIR] entering\r\n", 16, 10);

  if (HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_3) != HAL_OK) {
    HAL_UART_Transmit(&huart2, (uint8_t*)"[DIR] ERR start PWM\r\n", 22, 10);
    vTaskSuspend(NULL);
  }
  __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, DIR_CENTER);
  HAL_UART_Transmit(&huart2, (uint8_t*)"[DIR] up, CCR=1400\r\n", 19, 10);

  DirectionMsg msg;
  int lastDuty = -1;

  for (;;) {
    if (xQueueReceive(qDirection, &msg, portMAX_DELAY) == pdTRUE) {
      /* sens inversé validé en test: gauche/droite corrigés */
      int duty = (int)(DIR_CENTER - (int)(DIR_SCALE * msg.lx));
      int clamped = clamp_i(duty, DIR_MIN, DIR_MAX);
      if (clamped != duty) {
        char w[40]; int n = snprintf(w, sizeof w, "[DIR] clamp %d->%d\r\n", duty, clamped);
        HAL_UART_Transmit(&huart2, (uint8_t*)w, (uint16_t)n, 10);
        duty = clamped;
      }
      if (duty != lastDuty) {
        __HAL_TIM_SET_COMPARE(&htim2, TIM_CHANNEL_3, (uint16_t)duty);
        char l[32]; int n = snprintf(l, sizeof l, "[DIR] CCR=%d\r\n", duty);
        HAL_UART_Transmit(&huart2, (uint8_t*)l, (uint16_t)n, 10);
        lastDuty = duty;
      }
    }
  }
}

/* Vitesse : TIM3_CH1 / PB4 */
void StartSpdTask(void const * argument)
{
  HAL_UART_Transmit(&huart2, (uint8_t*)"[SPD] entering\r\n", 16, 10);

  if (HAL_TIM_PWM_Start(&htim3, TIM_CHANNEL_1) != HAL_OK) {
    HAL_UART_Transmit(&huart2, (uint8_t*)"[SPD] ERR start PWM\r\n", 22, 10);
    vTaskSuspend(NULL);
  }
  __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, SPD_CENTER);
  HAL_UART_Transmit(&huart2, (uint8_t*)"[SPD] up, CCR=1400\r\n", 19, 10);

  VitesseMsg msg;
  int lastDuty = -1;

  for (;;) {
    if (xQueueReceive(qVitesse, &msg, portMAX_DELAY) == pdTRUE) {
      float net = (msg.rt - msg.lt);                /* test_v1 */
      int duty  = (int)(SPD_CENTER + (int)(SPD_SCALE * net));
      int clamped = clamp_i(duty, SPD_MIN, SPD_MAX);
      if (clamped != duty) {
        char w[40]; int n = snprintf(w, sizeof w, "[SPD] clamp %d->%d\r\n", duty, clamped);
        HAL_UART_Transmit(&huart2, (uint8_t*)w, (uint16_t)n, 10);
        duty = clamped;
      }
      if (duty != lastDuty) {
        __HAL_TIM_SET_COMPARE(&htim3, TIM_CHANNEL_1, (uint16_t)duty);
        char l[32]; int n = snprintf(l, sizeof l, "[SPD] CCR=%d\r\n", duty);
        HAL_UART_Transmit(&huart2, (uint8_t*)l, (uint16_t)n, 10);
        lastDuty = duty;
      }
    }
  }
}

/* ==== Static allocation callbacks (si STATIC=1) ============================ */
#include "FreeRTOS.h"
static StaticTask_t xIdleTaskTCBBuffer;
static StackType_t  xIdleStack[configMINIMAL_STACK_SIZE];
void vApplicationGetIdleTaskMemory( StaticTask_t **ppxIdleTaskTCBBuffer,
                                    StackType_t  **ppxIdleTaskStackBuffer,
                                    uint32_t     *pulIdleTaskStackSize )
{
  *ppxIdleTaskTCBBuffer = &xIdleTaskTCBBuffer;
  *ppxIdleTaskStackBuffer = xIdleStack;
  *pulIdleTaskStackSize = configMINIMAL_STACK_SIZE;
}

#if ( configUSE_TIMERS == 1 )
static StaticTask_t xTimerTaskTCBBuffer;
static StackType_t  xTimerStack[ configTIMER_TASK_STACK_DEPTH ];
void vApplicationGetTimerTaskMemory( StaticTask_t **ppxTimerTaskTCBBuffer,
                                     StackType_t  **ppxTimerTaskStackBuffer,
                                     uint32_t     *pulTimerTaskStackSize )
{
  *ppxTimerTaskTCBBuffer = &xTimerTaskTCBBuffer;
  *ppxTimerTaskStackBuffer = xTimerStack;
  *pulTimerTaskStackSize = configTIMER_TASK_STACK_DEPTH;
}
#endif

/* ==== Hooks diag =========================================================== */
void vApplicationMallocFailedHook(void)
{
  const char* msg = "[ERR] malloc failed\r\n";
  HAL_UART_Transmit(&huart2,(uint8_t*)msg,(uint16_t)strlen(msg),50);
  taskDISABLE_INTERRUPTS(); for(;;){}
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
  char buf[64];
  int n = snprintf(buf,sizeof buf,"[ERR] stack overflow: %s\r\n", pcTaskName);
  HAL_UART_Transmit(&huart2,(uint8_t*)buf,(uint16_t)n,50);
  taskDISABLE_INTERRUPTS(); for(;;){}
}
