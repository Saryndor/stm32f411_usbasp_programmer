#pragma once

/* SPI1 Pin Definitions for STM32F411 */
// #define SPI_BASE        SPI1_BASE
// #define SPI_PORT        GPIOA
// #define SPI_SCK_PIN     GPIO5
// #define SPI_MISO_PIN    GPIO6
// #define SPI_MOSI_PIN    GPIO7
// #define SPI_AF          GPIO_AF5

// /* SPI1 peripheral clocks*/
// #define SPI_RCC_GPIO    RCC_GPIOA
// #define SPI_RCC_SPI     RCC_SPI1

/* SPI1 peripheral clocks*/
// #define SPI_RCC_GPIO    RCC_GPIOA
// #define SPI_RCC_SPI     RCC_SPI1

/* SPI2 Pin Definitions for STM32F411 */
#define SPI_BASE        SPI2_BASE
#define SPI_PORT        GPIOB
#define SPI_SCK_PIN     GPIO10
#define SPI_MISO_PIN    GPIO14
#define SPI_MOSI_PIN    GPIO15
#define SPI_AF          GPIO_AF5

/* SPI2 peripheral clocks*/
#define SPI_RCC_GPIO    RCC_GPIOB
#define SPI_RCC_SPI     RCC_SPI2



// #define CS_PORT         GPIOA
// #define CS_PIN          GPIO4
// #define CS_RCC_GPIO     RCC_GPIOA