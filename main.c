#include "stm32f3xx_hal.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

UART_HandleTypeDef huart2;

volatile int override_active = 0;
volatile int override_value = 0;

static void check_for_command(void)
{
    uint8_t rx_buf[16] = {0};
    HAL_StatusTypeDef status = HAL_UART_Receive(&huart2, rx_buf, sizeof(rx_buf) - 1, 10);

    if (status == HAL_OK || status == HAL_TIMEOUT)
    {
        if (rx_buf[0] != 0)
        { 
            int val;
            if (sscanf((char*)rx_buf, "SET %d", &val) == 1)
            {
                override_value = val;
                override_active = 1;
            }
            else if (strncmp((char *)rx_buf, "CLEAR", 5) == 0)
            {
                override_active = 0;
            }
        }
    }
}

void SysTick_Handler(void)
{
    HAL_IncTick();
}

static void UART2_Init(void)
{
    __HAL_RCC_USART2_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_2 | GPIO_PIN_15;
    gpio.Mode = GPIO_MODE_AF_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_HIGH;
    gpio.Alternate = GPIO_AF7_USART2;
    HAL_GPIO_Init(GPIOA, &gpio);

    huart2.Instance = USART2;
    huart2.Init.BaudRate = 115200;
    huart2.Init.WordLength = UART_WORDLENGTH_8B;
    huart2.Init.StopBits = UART_STOPBITS_1; 
    huart2.Init.Parity = UART_PARITY_NONE;
    huart2.Init.Mode = UART_MODE_TX_RX;
    huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
    HAL_UART_Init(&huart2);
}

static void send_string(const char *s)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)s, strlen(s), HAL_MAX_DELAY);
}

static void LED_Init(void)
{
    __HAL_RCC_GPIOB_CLK_ENABLE();
    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_3;
    gpio.Mode = GPIO_MODE_OUTPUT_PP;
    gpio.Pull = GPIO_NOPULL;
    gpio.Speed = GPIO_SPEED_FREQ_LOW;
    HAL_GPIO_Init(GPIOB, &gpio);
}

int main(void)
{
    HAL_Init();
    UART2_Init();
    LED_Init();

    float t = 0.0f;
    float actuator_position = 50.0f;
    const float GAIN = 0.2f;               // 0.2 default
    char msg[96];

    while (1)
    {
        check_for_command();

        int sensor_value;
        if (override_active)
        {
            sensor_value = override_value;
        }
        else
        {
            sensor_value = (int)(50.0f + 50.0f *sinf(t));
            sensor_value += (rand() % 11) - 5;
            if (sensor_value < 0) sensor_value = 0;
            if (sensor_value > 100) sensor_value = 100;
        }
        
        float target = (float)sensor_value;
        float error = target - actuator_position; 

        actuator_position += GAIN * error;

        sprintf(msg, "sensor: %d actuator: %d\r\n", sensor_value, (int)actuator_position);
        send_string(msg);

        // Control the LED based on actuator position
        if (actuator_position > 50.0f)
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_SET);
        else
            HAL_GPIO_WritePin(GPIOB, GPIO_PIN_3, GPIO_PIN_RESET);
            
        t += 0.15f;
        HAL_Delay(200);
    }
}
