#include "stm32f3xx_hal.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

UART_HandleTypeDef huart2;
ADC_HandleTypeDef hadc1;

volatile int override_active = 0;
volatile int override_value = 0;

static uint8_t rx_byte;
static char rx_command[32];
static volatile uint8_t rx_index = 0;

void SysTick_Handler(void)
{
    HAL_IncTick();
}

// Interrupt handler
void USART2_IRQHandler(void)
{
    HAL_UART_IRQHandler(&huart2);
}

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
    if (huart->Instance == USART2)
    {
        if (rx_byte == '\n' || rx_index >= sizeof(rx_command) - 1)
        {
            rx_command[rx_index] = '\0'; // Terminate the string

            int val;
            if (sscanf(rx_command, "SET %d", &val) == 1)
            {
                override_value = val;
                override_active = 1;
            }
            else if (strncmp(rx_command, "CLEAR", 5) == 0)
            {
                override_active = 0;
            }
            rx_index = 0; // Reset for next command
        }
        else
        {
            rx_command[rx_index++] = rx_byte;
        }
        HAL_UART_Receive_IT(&huart2, &rx_byte, 1);
    }
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

    // Enable the interrupt function in the chip's interrupt controller
    HAL_NVIC_SetPriority(USART2_IRQn, 1, 0);
    HAL_NVIC_EnableIRQ(USART2_IRQn);
    
    HAL_UART_Receive_IT(&huart2, &rx_byte, 1);
}


static void ADC_Init(void)
{
    __HAL_RCC_ADC1_CLK_ENABLE();
    __HAL_RCC_GPIOA_CLK_ENABLE();

    GPIO_InitTypeDef gpio = {0};
    gpio.Pin = GPIO_PIN_0;         // PA0
    gpio.Mode = GPIO_MODE_ANALOG;
    gpio.Pull = GPIO_NOPULL;
    HAL_GPIO_Init(GPIOA, &gpio);

    hadc1.Instance = ADC1;
    hadc1.Init.ClockPrescaler = ADC_CLOCK_ASYNC_DIV1;
    hadc1.Init.Resolution = ADC_RESOLUTION_12B;
    hadc1.Init.ScanConvMode = DISABLE;
    hadc1.Init.ContinuousConvMode = DISABLE;
    hadc1.Init.DiscontinuousConvMode = DISABLE;
    hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
    hadc1.Init.NbrOfConversion = 1;
    HAL_ADC_Init(&hadc1);

    ADC_ChannelConfTypeDef sConfig = {0};
    sConfig.Channel = ADC_CHANNEL_1;
    sConfig.Rank = ADC_REHULAR_RANK_1;
    sConfig.SamplingTime = ADC_SAMPLETIME_7CYCLES_5;
    HAL_ADC_ConfigChannel(&hadc, &sConfig);
}

static void read_sensor(void)
{
    HAL_ADC_Start(&hadc1);
    HAL_ADC_PollForConversion(&hadc1, HAL_MAX_DELAY);
    uint32_t raw = HAL_ADC_GetValue(&hadc1);
    HAL_ADC_Stop(&hadc1);

    return (int)((raw*100) / 4095);
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

static void send_string(const char *s)
{
    HAL_UART_Transmit(&huart2, (uint8_t *)s, strlen(s), HAL_MAX_DELAY);
}

int main(void)
{
    HAL_Init();
    UART2_Init();
    ADC1_Init();
    LED_Init();

    float actuator_position = 50.0f;
    const float GAIN = 0.2f;               // 0.2 default
    char msg[96];

    while (1)
    {
        int sensor_value;
        if (override_active)
        {
            sensor_value = override_value;
        }
        else
        {
            sensor_value = read_sensor();       // ADC reading from Analog Discovery 2
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
            
        HAL_Delay(200);
    }
}
