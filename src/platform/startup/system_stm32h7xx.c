/**
  ******************************************************************************
  * @file    system_stm32h7xx.c
  * @author  MCD Application Team
  * @brief   CMSIS Cortex-Mx Device Peripheral Access Layer System Source File.
  *
  *   This file provides two functions and one global variable to be called from
  *   user application:
  *      - ExitRun0Mode(): Specifies the Power Supply source. This function is
  *                        called at startup just after reset and before the call
  *                        of SystemInit(). This call is made inside
  *                        the "startup_stm32h7xx.s" file.
  *
  *      - SystemInit(): This function is called at startup just after reset and
  *                      before branch to main program. This call is made inside
  *                      the "startup_stm32h7xx.s" file.
  *
  *      - SystemCoreClock variable: Contains the core clock (HCLK), it can be used
  *                                  by the user application to setup the SysTick
  *                                  timer or configure other parameters.
  *
  *      - SystemCoreClockUpdate(): Updates the variable SystemCoreClock and must
  *                                 be called whenever the core clock is changed
  *                                 during program execution.
  *
  *
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2017 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */

/** @addtogroup CMSIS
  * @{
  */

/** @addtogroup stm32h7xx_system
  * @{
  */

/** @addtogroup STM32H7xx_System_Private_Includes
  * @{
  */

#include <string.h>

#include "platform.h"

// #include "drivers/memprot.h"
#include "drivers/system.h"

#include "system_stm32h7xx.h"

#if !defined  (HSE_VALUE)
  #define HSE_VALUE    ((uint32_t)25000000) /*!< Value of the External oscillator in Hz */
#endif /* HSE_VALUE */

#if !defined  (CSI_VALUE)
  #define CSI_VALUE    ((uint32_t)4000000) /*!< Value of the Internal oscillator in Hz*/
#endif /* CSI_VALUE */

#if !defined  (HSI_VALUE)
  #define HSI_VALUE    ((uint32_t)64000000) /*!< Value of the Internal oscillator in Hz*/
#endif /* HSI_VALUE */

/**
  * @}
  */

/** @addtogroup STM32H7xx_System_Private_TypesDefinitions
  * @{
  */

/**
  * @}
  */

/** @addtogroup STM32H7xx_System_Private_Defines
  * @{
  */

/************************* Miscellaneous Configuration ************************/
/*!< Uncomment the following line if you need to use initialized data in D2 domain SRAM (AHB SRAM) */
/* #define DATA_IN_D2_SRAM */

/*!< Uncomment the following line if you need to use external SRAM or SDRAM mounted
      on EVAL board as data memory  */
/* #define DATA_IN_ExtSRAM */
#define DATA_IN_ExtSDRAM

#if defined(DATA_IN_ExtSRAM) && defined(DATA_IN_ExtSDRAM)
#error "Please select DATA_IN_ExtSRAM or DATA_IN_ExtSDRAM "
#endif /* DATA_IN_ExtSRAM && DATA_IN_ExtSDRAM */

/* Note: Following vector table addresses must be defined in line with linker
          configuration. */
/*!< Uncomment the following line if you need to relocate the vector table
      anywhere in FLASH BANK1 or AXI SRAM, else the vector table is kept at the automatic
      remap of boot address selected */
/* #define USER_VECT_TAB_ADDRESS */

#if defined(USER_VECT_TAB_ADDRESS)
/*!< Uncomment the following line if you need to relocate your vector Table
      in D1 AXI SRAM else user remap will be done in FLASH BANK1. */
/* #define VECT_TAB_SRAM */
#if defined(VECT_TAB_SRAM)
#define VECT_TAB_BASE_ADDRESS   D1_AXISRAM_BASE   /*!< Vector Table base address field.
                                                        This value must be a multiple of 0x400. */
#define VECT_TAB_OFFSET         0x00000000U       /*!< Vector Table base offset field.
                                                        This value must be a multiple of 0x400. */
#else
#define VECT_TAB_BASE_ADDRESS   FLASH_BANK1_BASE  /*!< Vector Table base address field.
                                                        This value must be a multiple of 0x400. */
#define VECT_TAB_OFFSET         0x00000000U       /*!< Vector Table base offset field.
                                                        This value must be a multiple of 0x400. */
#endif /* VECT_TAB_SRAM */
#endif /* USER_VECT_TAB_ADDRESS */
/******************************************************************************/

/**
  * @}
  */

/** @addtogroup STM32H7xx_System_Private_Macros
  * @{
  */

/**
  * @}
  */

/** @addtogroup STM32H7xx_System_Private_Variables
  * @{
  */
  /* This variable is updated in three ways:
      1) by calling CMSIS function SystemCoreClockUpdate()
      2) by calling HAL API function HAL_RCC_GetHCLKFreq()
      3) each time HAL_RCC_ClockConfig() is called to configure the system clock frequency
          Note: If you use this function to configure the system clock; then there
                is no need to call the 2 first functions listed above, since SystemCoreClock
                variable is updated automatically.
  */
  uint32_t SystemCoreClock = 64000000;
  uint32_t SystemD2Clock = 64000000;
  const  uint8_t D1CorePrescTable[16] = {0, 0, 0, 0, 1, 2, 3, 4, 1, 2, 3, 4, 6, 7, 8, 9};

/**
  * @}
  */

/** @addtogroup STM32H7xx_System_Private_FunctionPrototypes
  * @{
  */
#if defined(DATA_IN_ExtSRAM) || defined(DATA_IN_ExtSDRAM)
static void SystemInit_ExtMemCtl(void);
#endif /* DATA_IN_ExtSRAM || DATA_IN_ExtSDRAM */

/**
  * @}
  */

/** @addtogroup STM32H7xx_System_Private_Functions
  * @{
  */

static void ErrorHandler(void)
{
    while (1);
}

static void HandleStuckSysTick(void)
{
    uint32_t tickStart = HAL_GetTick();
    uint32_t tickEnd = 0;

    // H7 at 480Mhz requires a loop count of 160000. Double this for the timeout to be safe.
    int attemptsRemaining = 320000;
    while (((tickEnd = HAL_GetTick()) == tickStart) && --attemptsRemaining) {
    }

    if (tickStart == tickEnd) {
        // systemResetWithoutDisablingCaches();
    }
}

// Workaround for weird HSE behaviors
// (Observed only on Rev.V H750, but may also apply to H743 and Rev.V.)
#define USE_H7_HSERDY_SLOW_WORKAROUND
#define USE_H7_HSE_TIMEOUT_WORKAROUND

/**
  * @brief  Setup the microcontroller system
  *         Initialize the FPU setting and  vector table location
  *         configuration.
  * @param  None
  * @retval None
  */

/**
  * @brief  System Clock Configuration
  *         The system Clock is configured as follow : 
  *            System Clock source            = PLL (HSE)
  *            SYSCLK(Hz)                     = 480000000 (CPU Clock)
  *            HCLK(Hz)                       = 240000000 (AXI and AHBs Clock)
  *            AHB Prescaler                  = 2
  *            D1 APB3 Prescaler              = 2 (APB3 Clock  240MHz)
  *            D2 APB1 Prescaler              = 2 (APB1 Clock  240MHz)
  *            D2 APB2 Prescaler              = 2 (APB2 Clock  240MHz)
  *            D3 APB4 Prescaler              = 2 (APB4 Clock  240MHz)
  *            HSE Frequency(Hz)              = 25000000
  *            PLL1_M                         = 5
  *            PLL1_N                         = 192
  *            PLL1_P                         = 2
  *            PLL1_Q                         = 8
  *            PLL1_R                         = 5
  *            PLL3_M                         = 5
  *            PLL3_N                         = 192
  *            PLL3_Q                         = 20
  *            VDD(V)                         = 3.3
  *            Flash Latency(WS)              = 4
  * @param  None
  * @retval None
  */
static void SystemClockHSE_Config(void)
{
    RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
    RCC_OscInitTypeDef RCC_OscInitStruct = {0};

#ifdef notdef
    // CSI has been disabled at SystemInit().
    // HAL_RCC_ClockConfig() will fail because CSIRDY is off.

    /* -1- Select CSI as system clock source to allow modification of the PLL configuration */

    RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_SYSCLK;
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_CSI;
    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_1) != HAL_OK) {
        /* Initialization Error */
        ErrorHandler();
    }
#endif

    // Configure voltage scale.
    // It has been pre-configured at PWR_REGULATOR_VOLTAGE_SCALE1,
    // and it may stay or overridden by PWR_REGULATOR_VOLTAGE_SCALE0 depending on the clock config.

    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE0);

    while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {
        // Empty
    }

    /* -2- Enable HSE Oscillator, select it as PLL source and finally activate the PLL */

#ifdef USE_H7_HSERDY_SLOW_WORKAROUND
    // With reference to 2.3.22 in the ES0250 Errata for the L476.
    // Applying the same workaround here in the vain hopes that it improves startup times.
    // Randomly the HSERDY bit takes AGES, over 10 seconds, to be set.

    __HAL_RCC_GPIOH_CLK_ENABLE();

    HAL_GPIO_WritePin(GPIOH, GPIO_PIN_0 | GPIO_PIN_1, GPIO_PIN_RESET);

    GPIO_InitTypeDef  gpio_initstruct;
    gpio_initstruct.Pin = GPIO_PIN_0 | GPIO_PIN_1;
    gpio_initstruct.Mode = GPIO_MODE_OUTPUT_PP;
    gpio_initstruct.Pull = GPIO_NOPULL;
    gpio_initstruct.Speed = GPIO_SPEED_FREQ_VERY_HIGH;

    HAL_GPIO_Init(GPIOH, &gpio_initstruct);
#endif

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    RCC_OscInitStruct.HSEState = RCC_HSE_ON;

    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
    RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    RCC_OscInitStruct.PLL.PLLM = 5;
    RCC_OscInitStruct.PLL.PLLN = 192; // Holds DIVN's output to DIVP at 960Mhz.
    RCC_OscInitStruct.PLL.PLLP = 2;
    // Dividing PLLQ here by 8 keeps PLLQ1, used for SPI, below 200Mhz, required per the spec.
    RCC_OscInitStruct.PLL.PLLQ = 8;
    RCC_OscInitStruct.PLL.PLLR = 5;

    RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
    RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_2;
    HAL_StatusTypeDef status = HAL_RCC_OscConfig(&RCC_OscInitStruct);

#ifdef USE_H7_HSE_TIMEOUT_WORKAROUND
    if (status == HAL_TIMEOUT) {
        // systemResetWithoutDisablingCaches(); // DC - sometimes HSERDY gets stuck, waiting longer doesn't help.
    }
#endif

    if (status != HAL_OK) {
        /* Initialization Error */
        ErrorHandler();
    }

    // Configure PLL2 and PLL3
    // Use of PLL2 and PLL3 are not determined yet.
    // A review of total system wide clock requirements is necessary.

    // Configure SCGU (System Clock Generation Unit)
    // Select PLL as system clock source and configure bus clock dividers.
    //
    // Clock type and divider member names do not have direct visual correspondence.
    // Here is how these correspond:
    //   RCC_CLOCKTYPE_SYSCLK           sys_ck
    //   RCC_CLOCKTYPE_HCLK             AHBx (rcc_hclk1,rcc_hclk2,rcc_hclk3,rcc_hclk4)
    //   RCC_CLOCKTYPE_D1PCLK1          APB3 (rcc_pclk3)
    //   RCC_CLOCKTYPE_PCLK1            APB1 (rcc_pclk1)
    //   RCC_CLOCKTYPE_PCLK2            APB2 (rcc_pclk2)
    //   RCC_CLOCKTYPE_D3PCLK1          APB4 (rcc_pclk4)

    RCC_ClkInitStruct.ClockType = ( \
        RCC_CLOCKTYPE_SYSCLK | \
        RCC_CLOCKTYPE_HCLK | \
        RCC_CLOCKTYPE_D1PCLK1 | \
        RCC_CLOCKTYPE_PCLK1 | \
        RCC_CLOCKTYPE_PCLK2 | \
        RCC_CLOCKTYPE_D3PCLK1);
    RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;

    RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
    RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
    RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
    RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
    RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

    if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_4) != HAL_OK) {
        /* Initialization Error */
        ErrorHandler();
    }

    /* -4- Optional: Disable CSI Oscillator (if the HSI is no more needed by the application)*/
    RCC_OscInitStruct.OscillatorType  = RCC_OSCILLATORTYPE_CSI;
    RCC_OscInitStruct.CSIState        = RCC_CSI_OFF;
    RCC_OscInitStruct.PLL.PLLState    = RCC_PLL_NONE;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        /* Initialization Error */
        ErrorHandler();
    }
}

static void SystemClockLSE_Config(void)
{
    // Configure LSE Drive Capability
    HAL_PWR_EnableBkUpAccess();
    __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_HIGH);

    RCC_OscInitTypeDef RCC_OscInitStruct = {0};

    RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_LSE;
    RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
    RCC_OscInitStruct.LSEState = RCC_LSE_ON;
    RCC_OscInitStruct.LSIState = RCC_LSI_OFF;
    if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
        /* Initialization Error */
        ErrorHandler();
    }
}

void SystemClock_Config(void)
{
    // Configure power supply

    HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

    // Pre-configure voltage scale to PWR_REGULATOR_VOLTAGE_SCALE1.
    // SystemClockHSE_Config may configure PWR_REGULATOR_VOLTAGE_SCALE0.

    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

    while (!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {
        // Empty
    }

    SystemClockHSE_Config();
    SystemClockLSE_Config();

    /*activate CSI clock mondatory for I/O Compensation Cell*/

    __HAL_RCC_CSI_ENABLE() ;

    /* Enable SYSCFG clock mondatory for I/O Compensation Cell */

    __HAL_RCC_SYSCFG_CLK_ENABLE() ;

    /* Enables the I/O Compensation Cell */

    HAL_EnableCompensationCell();

    HandleStuckSysTick();

    HAL_Delay(10);

    // Configure peripheral clocks

    RCC_PeriphCLKInitTypeDef RCC_PeriphClkInit;

    // Configure HSI48 as peripheral clock for USB

    RCC_PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USB;
    RCC_PeriphClkInit.UsbClockSelection = RCC_USBCLKSOURCE_HSI48;
    HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphClkInit);

    // Configure CRS for dynamic calibration of HSI48
    // While ES0392 Rev 5 "STM32H742xI/G and STM32H743xI/G device limitations" states CRS not working for REV.Y,
    // it is always turned on as it seems that it has no negative effect on clock accuracy.

    RCC_CRSInitTypeDef crsInit = {
        .Prescaler = RCC_CRS_SYNC_DIV1,
        .Source = RCC_CRS_SYNC_SOURCE_USB2,
        .Polarity = RCC_CRS_SYNC_POLARITY_RISING,
        .ReloadValue = RCC_CRS_RELOADVALUE_DEFAULT,
        .ErrorLimitValue = RCC_CRS_ERRORLIMIT_DEFAULT,
        .HSI48CalibrationValue = RCC_CRS_HSI48CALIBRATION_DEFAULT,
    };

    __HAL_RCC_CRS_CLK_ENABLE();
    HAL_RCCEx_CRSConfig(&crsInit);

#ifdef USE_CRS_INTERRUPTS
    // Turn on USE_CRS_INTERRUPTS to see CRS in action
    HAL_NVIC_SetPriority(CRS_IRQn, 6, 0);
    HAL_NVIC_EnableIRQ(CRS_IRQn);
    __HAL_RCC_CRS_ENABLE_IT(RCC_CRS_IT_SYNCOK|RCC_CRS_IT_SYNCWARN|RCC_CRS_IT_ESYNC|RCC_CRS_IT_ERR);
#endif

    // Configure UART peripheral clock sources
    //
    // Possible sources:
    //   D2PCLK1 (pclk1 for APB1 = USART234578)
    //   D2PCLK2 (pclk2 for APB2 = USART16)
    //   PLL2 (pll2_q_ck)
    //   PLL3 (pll3_q_ck)
    //   HSI (hsi_ck)
    //   CSI (csi_ck)
    //   LSE (lse_ck)

    RCC_PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_USART16|RCC_PERIPHCLK_USART234578;
    RCC_PeriphClkInit.Usart16ClockSelection = RCC_USART16CLKSOURCE_D2PCLK2;
    RCC_PeriphClkInit.Usart234578ClockSelection = RCC_USART234578CLKSOURCE_D2PCLK1;
    HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphClkInit);

    // Configure QSPI peripheral clock sources
    //
    // Possible sources for QSPI:
    //   D1HCLK (hclk for AHB1)
    //   PLL1 (pll1_q_ck)
    //   PLL2 (pll2_r_ck)
    //   CLKP (per_ck)

    RCC_PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_QSPI;
    RCC_PeriphClkInit.QspiClockSelection = RCC_QSPICLKSOURCE_D1HCLK;
    HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphClkInit);

    // Configure SPI peripheral clock sources
    //
    // Possible sources for SPI123:
    //   PLL (pll1_q_ck)
    //   PLL2 (pll2_p_ck)
    //   PLL3 (pll3_p_ck)
    //   PIN (I2S_CKIN)
    //   CLKP (per_ck)
    // Possible sources for SPI45:
    //   D2PCLK1 (rcc_pclk2 = APB1) 100MHz
    //   PLL2 (pll2_q_ck)
    //   PLL3 (pll3_q_ck)
    //   HSI (hsi_ker_ck)
    //   CSI (csi_ker_ck)
    //   HSE (hse_ck)
    // Possible sources for SPI6:
    //   D3PCLK1 (rcc_pclk4 = APB4) 100MHz
    //   PLL2 (pll2_q_ck)
    //   PLL3 (pll3_q_ck)
    //   HSI (hsi_ker_ck)
    //   CSI (csi_ker_ck)
    //   HSE (hse_ck)

    // We use 100MHz for Rev.Y and 120MHz for Rev.V from various sources

    RCC_PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_SPI123|RCC_PERIPHCLK_SPI45|RCC_PERIPHCLK_SPI6;
    RCC_PeriphClkInit.Spi123ClockSelection = RCC_SPI123CLKSOURCE_PLL;
    RCC_PeriphClkInit.Spi45ClockSelection = RCC_SPI45CLKSOURCE_D2PCLK1;
    RCC_PeriphClkInit.Spi6ClockSelection = RCC_SPI6CLKSOURCE_D3PCLK1;
    HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphClkInit);

    // Configure I2C peripheral clock sources
    //
    // Current source for I2C123:
    //   D2PCLK1 (rcc_pclk1 = APB1 peripheral clock)
    //
    // Current source for I2C4:
    //   D3PCLK1 (rcc_pclk4 = APB4 peripheral clock)
    //
    // Note that peripheral clock determination in bus_i2c_hal_init.c must be modified when the sources are modified.

    RCC_PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_I2C123|RCC_PERIPHCLK_I2C4;
    RCC_PeriphClkInit.I2c123ClockSelection = RCC_I2C123CLKSOURCE_D2PCLK1;
    RCC_PeriphClkInit.I2c4ClockSelection = RCC_I2C4CLKSOURCE_D3PCLK1;
    HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphClkInit);

#ifdef USE_SDCARD_SDIO
    __HAL_RCC_SDMMC1_CLK_ENABLE(); // FIXME enable SDMMC1 or SDMMC2 depending on target.

    RCC_PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_SDMMC;

    RCC_PeriphClkInit.PLL2.PLL2M = 5;
    RCC_PeriphClkInit.PLL2.PLL2N = 160; // Oscillator Frequency / 5 (PLL2M) = 5 * this value (PLL2N) = 800Mhz.
    RCC_PeriphClkInit.PLL2.PLL2VCOSEL = RCC_PLL2VCOWIDE; // Wide VCO range:192 to 836 MHz
    RCC_PeriphClkInit.PLL2.PLL2RGE = RCC_PLL2VCIRANGE_2; // PLL2 input between 4 and 8Mhz (5)
    RCC_PeriphClkInit.PLL2.PLL2FRACN = 0;

    RCC_PeriphClkInit.PLL2.PLL2P = 2; // 800Mhz / 2 = 400Mhz
    RCC_PeriphClkInit.PLL2.PLL2Q = 3; // 800Mhz / 3 = 266Mhz // 133Mhz can be derived from this for for QSPI if flash chip supports the speed.
    RCC_PeriphClkInit.PLL2.PLL2R = 4; // 800Mhz / 4 = 200Mhz // HAL LIBS REQUIRE 200MHZ SDMMC CLOCK, see HAL_SD_ConfigWideBusOperation, SDMMC_HSpeed_CLK_DIV, SDMMC_NSpeed_CLK_DIV

    RCC_PeriphClkInit.SdmmcClockSelection = RCC_SDMMCCLKSOURCE_PLL2;
    HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphClkInit);
#  endif

    RCC_PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
    RCC_PeriphClkInit.AdcClockSelection = RCC_ADCCLKSOURCE_CLKP;
    HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphClkInit);

    // Configure RTC peripheral clock sources
    //
    // Possible sources for RTC:
    //    LSE (lse_ck)
    //    LSI (lsi_ck)
    //    HSE (hse_divx_ck)
    RCC_PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_RTC;
    RCC_PeriphClkInit.RTCClockSelection = RCC_RTCCLKSOURCE_LSE;
    HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphClkInit);

    // Configure MCO clocks for clock test/verification

    // Possible sources for MCO1:
    //   RCC_MCO1SOURCE_HSI (hsi_ck)
    //   RCC_MCO1SOURCE_LSE (?)
    //   RCC_MCO1SOURCE_HSE (hse_ck)
    //   RCC_MCO1SOURCE_PLL1QCLK (pll1_q_ck)
    //   RCC_MCO1SOURCE_HSI48 (hsi48_ck)

    //  HAL_RCC_MCOConfig(RCC_MCO1, RCC_MCO1SOURCE_HSE, RCC_MCODIV_1);     // HSE(8M) / 1 = 1M
    HAL_RCC_MCOConfig(RCC_MCO1, RCC_MCO1SOURCE_HSI48, RCC_MCODIV_4);     // HSI48(48M) / 4 = 12M

    // Possible sources for MCO2:
    //   RCC_MCO2SOURCE_SYSCLK  (sys_ck)
    //   RCC_MCO2SOURCE_PLL2PCLK (pll2_p_ck)
    //   RCC_MCO2SOURCE_HSE (hse_ck)
    //   RCC_MCO2SOURCE_PLLCLK (pll1_p_ck)
    //   RCC_MCO2SOURCE_CSICLK (csi_ck)
    //   RCC_MCO2SOURCE_LSICLK (lsi_ck)

    HAL_RCC_MCOConfig(RCC_MCO2, RCC_MCO2SOURCE_PLLCLK, RCC_MCODIV_15); // PLL1P(400M) / 15 = 26.67M
}

#ifdef USE_CRS_INTERRUPTS
static uint32_t crs_syncok = 0;
static uint32_t crs_syncwarn = 0;
static uint32_t crs_expectedsync = 0;
static uint32_t crs_error = 0;

void HAL_RCCEx_CRS_SyncOkCallback(void)
{
    ++crs_syncok;
}

void HAL_RCCEx_CRS_SyncWarnCallback(void)
{
    ++crs_syncwarn;
}

void HAL_RCCEx_CRS_ExpectedSyncCallback(void)
{
    ++crs_expectedsync;
}

void HAL_RCCEx_CRS_ErrorCallback(uint32_t Error)
{
    ++crs_error;
}

void CRS_IRQHandler(void)
{
    HAL_RCCEx_CRS_IRQHandler();
}
#endif

void SystemInit (void)
{
#if defined (DATA_IN_D2_SRAM)
    __IO uint32_t tmpreg;
#endif /* DATA_IN_D2_SRAM */

    // FPU settings
#if (__FPU_PRESENT == 1) && (__FPU_USED == 1)
    SCB->CPACR |= ((3UL << (10*2))|(3UL << (11*2)));  // Set CP10 and CP11 Full Access
#endif
    /* Reset the RCC clock configuration to the default reset state ------------*/

    /* Increasing the CPU frequency */
    if(FLASH_LATENCY_DEFAULT > (READ_BIT((FLASH->ACR), FLASH_ACR_LATENCY)))
    {
        /* Program the new number of wait states to the LATENCY bits in the FLASH_ACR register */
        MODIFY_REG(FLASH->ACR, FLASH_ACR_LATENCY, (uint32_t)(FLASH_LATENCY_DEFAULT));
    }

    // Reset the RCC clock configuration to the default reset state
    // Set HSION bit
    RCC->CR = RCC_CR_HSION;

    // Reset CFGR register
    RCC->CFGR = 0x00000000;

    // Reset HSEON, HSECSSON, CSION, HSI48ON, CSIKERON, PLL1ON, PLL2ON and PLL3ON bits

    // XXX Don't do this until we are established with clock handling
    // RCC->CR &= 0xEAF6ED7FU;

    // Instead, we explicitly turn those on
    RCC->CR |= RCC_CR_CSION;
    RCC->CR |= RCC_CR_HSION;
    RCC->CR |= RCC_CR_HSEON;
    RCC->CR |= RCC_CR_HSI48ON;

    /* Decreasing the number of wait states because of lower CPU frequency */
    if(FLASH_LATENCY_DEFAULT < (READ_BIT((FLASH->ACR), FLASH_ACR_LATENCY)))
    {
        /* Program the new number of wait states to the LATENCY bits in the FLASH_ACR register */
        MODIFY_REG(FLASH->ACR, FLASH_ACR_LATENCY, (uint32_t)(FLASH_LATENCY_DEFAULT));
    }

#if defined(D3_SRAM_BASE)
    /* Reset D1CFGR register */
    RCC->D1CFGR = 0x00000000;

    /* Reset D2CFGR register */
    RCC->D2CFGR = 0x00000000;

    /* Reset D3CFGR register */
    RCC->D3CFGR = 0x00000000;
#else
    /* Reset CDCFGR1 register */
    RCC->CDCFGR1 = 0x00000000;

    /* Reset CDCFGR2 register */
    RCC->CDCFGR2 = 0x00000000;

    /* Reset SRDCFGR register */
    RCC->SRDCFGR = 0x00000000;
#endif

    /* Reset PLLCKSELR register */
    RCC->PLLCKSELR = 0x00000000;

    /* Reset PLLCFGR register */
    RCC->PLLCFGR = 0x00000000;
    /* Reset PLL1DIVR register */
    RCC->PLL1DIVR = 0x00000000;
    /* Reset PLL1FRACR register */
    RCC->PLL1FRACR = 0x00000000;

    /* Reset PLL2DIVR register */
    RCC->PLL2DIVR = 0x00000000;
    /* Reset PLL2FRACR register */
    RCC->PLL2FRACR = 0x00000000;

    /* Reset PLL3DIVR register */
    RCC->PLL3DIVR = 0x00000000;
    /* Reset PLL3FRACR register */
    RCC->PLL3FRACR = 0x00000000;

    /* Reset HSEBYP bit */
    RCC->CR &= 0xFFFBFFFFU;

    /* Disable all interrupts */
    RCC->CIER = 0x00000000;

#if (STM32H7_DEV_ID == 0x450UL)
  /* single core line */
  if((DBGMCU->IDCODE & 0xFFFF0000U) < 0x20000000U)
  {
    /* if stm32h7 revY*/
    /* Change the switch matrix read issuing capability to 1 for the AXI SRAM target (Target 7) */
    *((__IO uint32_t*)0x51008108) = 0x00000001U;
  }
#endif /* STM32H7_DEV_ID */

#if defined (DATA_IN_D2_SRAM)
    /* in case of initialized data in D2 SRAM (AHB SRAM), enable the D2 SRAM clock (AHB SRAM clock) */
#if defined(RCC_AHB2ENR_D2SRAM3EN)
    RCC->AHB2ENR |= (RCC_AHB2ENR_D2SRAM1EN | RCC_AHB2ENR_D2SRAM2EN | RCC_AHB2ENR_D2SRAM3EN);
#elif defined(RCC_AHB2ENR_D2SRAM2EN)
    RCC->AHB2ENR |= (RCC_AHB2ENR_D2SRAM1EN | RCC_AHB2ENR_D2SRAM2EN);
#else
    RCC->AHB2ENR |= (RCC_AHB2ENR_AHBSRAM1EN | RCC_AHB2ENR_AHBSRAM2EN);
#endif /* RCC_AHB2ENR_D2SRAM3EN */

    tmpreg = RCC->AHB2ENR;
    (void) tmpreg;
#endif /* DATA_IN_D2_SRAM */

    /*
    * Disable the FMC bank1 (enabled after reset).
    * This, prevents CPU speculation access on this bank which blocks the use of FMC during
    * 24us. During this time the others FMC master (such as LTDC) cannot use it!
    */
    FMC_Bank1_R->BTCR[0] = 0x000030D2;

#if defined(DATA_IN_ExtSRAM) || defined(DATA_IN_ExtSDRAM)
    SystemInit_ExtMemCtl();
#endif /* DATA_IN_ExtSRAM || DATA_IN_ExtSDRAM */

    /* Configure the Vector Table location add offset address ------------------*/
#if defined(USER_VECT_TAB_ADDRESS)
    SCB->VTOR = VECT_TAB_BASE_ADDRESS | VECT_TAB_OFFSET; /* Vector Table Relocation in Internal D1 AXI-RAM or in Internal FLASH */
#elif defined(USE_EXST)
    extern uint8_t isr_vector_table_base;

    SCB->VTOR = (uint32_t)&isr_vector_table_base;
#endif /* USER_VECT_TAB_ADDRESS */
}

void SystemSetup(void)
{
    // memProtReset();

#if !defined(USE_EXST)
    // only stand-alone and bootloader firmware needs to do this.
    // if it's done in the EXST firmware as well as the BOOTLOADER firmware you get a reset loop.
    // systemProcessResetReason();
#endif

#ifdef USE_HAL_DRIVER
    HAL_Init();
#endif

    SystemClock_Config();
    SystemCoreClockUpdate();

    // Configure MPU

    // memProtConfigure();

    // Enable CPU L1-Cache
    SCB_EnableICache();
    SCB_EnableDCache();
}

/**
  * @brief  Update SystemCoreClock variable according to Clock Register Values.
  *         The SystemCoreClock variable contains the core clock , it can
  *         be used by the user application to setup the SysTick timer or configure
  *         other parameters.
  *
  * @note   Each time the core clock changes, this function must be called
  *         to update SystemCoreClock variable value. Otherwise, any configuration
  *         based on this variable will be incorrect.
  *
  * @note   - The system frequency computed by this function is not the real
  *           frequency in the chip. It is calculated based on the predefined
  *           constant and the selected clock source:
  *
  *           - If SYSCLK source is CSI, SystemCoreClock will contain the CSI_VALUE(*)
  *           - If SYSCLK source is HSI, SystemCoreClock will contain the HSI_VALUE(**)
  *           - If SYSCLK source is HSE, SystemCoreClock will contain the HSE_VALUE(***)
  *           - If SYSCLK source is PLL, SystemCoreClock will contain the CSI_VALUE(*),
  *             HSI_VALUE(**) or HSE_VALUE(***) multiplied/divided by the PLL factors.
  *
  *         (*) CSI_VALUE is a constant defined in stm32h7xx_hal.h file (default value
  *             4 MHz) but the real value may vary depending on the variations
  *             in voltage and temperature.
  *         (**) HSI_VALUE is a constant defined in stm32h7xx_hal.h file (default value
  *             64 MHz) but the real value may vary depending on the variations
  *             in voltage and temperature.
  *
  *         (***)HSE_VALUE is a constant defined in stm32h7xx_hal.h file (default value
  *              25 MHz), user has to ensure that HSE_VALUE is same as the real
  *              frequency of the crystal used. Otherwise, this function may
  *              have wrong result.
  *
  *         - The result of this function could be not correct when using fractional
  *           value for HSE crystal.
  * @param  None
  * @retval None
  */
void SystemCoreClockUpdate (void)
{
    SystemCoreClock = HAL_RCC_GetSysClockFreq();
}

#if defined(DATA_IN_ExtSRAM) || defined(DATA_IN_ExtSDRAM)
/**
  * @brief  Setup the external memory controller.
  *         Called in startup_stm32h7xx.s before jump to main.
  *         This function configures the external memories (SRAM/SDRAM)
  *         This SRAM/SDRAM will be used as program data memory (including heap and stack).
  * @param  None
  * @retval None
  */
void SystemInit_ExtMemCtl(void)
{
#if defined(DATA_IN_ExtSDRAM)
    register uint32_t tmpreg = 0, timeout = 0xFFFF;
    register __IO uint32_t index;

    /* Enable GPIOC, GPIOD, GPIOE, GPIOF, GPIOG, GPIOH and GPIOI interface clock */
    RCC->AHB4ENR  |= 0x000001FC;
    /* Connect PCx pins to FMC Alternate function */
    GPIOC->AFR[0]  = 0x0000000C;
    GPIOC->AFR[1]  = 0x00000000;
    /* Configure PCx pins in Alternate function mode */
    GPIOC->MODER   = 0xFFFFFFFE;
    /* Configure PCx pins speed to 50 MHz */
    GPIOC->OSPEEDR = 0x00000003;
    /* Configure PCx pins Output type to push-pull */
    GPIOC->OTYPER  = 0x00000000;
    /* No pull-up, pull-down for PCx pins */
    GPIOC->PUPDR   = 0x00000001;
    /* Connect PDx pins to FMC Alternate function */
    GPIOD->AFR[0]  = 0x000000CC;
    GPIOD->AFR[1]  = 0xCC000CCC;
    /* Configure PDx pins in Alternate function mode */
    GPIOD->MODER   = 0xAFEAFFFA;
    /* Configure PDx pins speed to 50 MHz */
    GPIOD->OSPEEDR = 0xF03F000F;
    /* Configure PDx pins Output type to push-pull */
    GPIOD->OTYPER  = 0x00000000;
    /* No pull-up, pull-down for PDx pins */
    GPIOD->PUPDR   = 0x50150005;
    /* Connect PEx pins to FMC Alternate function */
    GPIOE->AFR[0]  = 0xC00000CC;
    GPIOE->AFR[1]  = 0xCCCCCCCC;
      /* Configure PEx pins in Alternate function mode */
    GPIOE->MODER   = 0xAAAABFFA;
    /* Configure PEx pins speed to 50 MHz */
    GPIOE->OSPEEDR = 0xFFFFC00F;
    /* Configure PEx pins Output type to push-pull */
    GPIOE->OTYPER  = 0x00000000;
    /* No pull-up, pull-down for PEx pins */
    GPIOE->PUPDR   = 0x55554005;
    /* Connect PFx pins to FMC Alternate function */
    GPIOF->AFR[0]  = 0x00CCCCCC;
    GPIOF->AFR[1]  = 0xCCCCC000;
    /* Configure PFx pins in Alternate function mode */
    GPIOF->MODER   = 0xAABFFAAA;
    /* Configure PFx pins speed to 50 MHz */
    GPIOF->OSPEEDR = 0xFFC00FFF;
    /* Configure PFx pins Output type to push-pull */
    GPIOF->OTYPER  = 0x00000000;
    /* No pull-up, pull-down for PFx pins */
    GPIOF->PUPDR   = 0x55400555;
    /* Connect PGx pins to FMC Alternate function */
    GPIOG->AFR[0]  = 0x00CC0CCC;
    GPIOG->AFR[1]  = 0xC000000C;
    /* Configure PGx pins in Alternate function mode */
    GPIOG->MODER   = 0xBFFEFAEA;
  /* Configure PGx pins speed to 50 MHz */
    GPIOG->OSPEEDR = 0xC0030F3F;
    /* Configure PGx pins Output type to push-pull */
    GPIOG->OTYPER  = 0x00000000;
    /* No pull-up, pull-down for PGx pins */
    GPIOG->PUPDR   = 0x40010515;
    /* Connect PHx pins to FMC Alternate function */
    GPIOH->AFR[0]  = 0xCC000000;
    GPIOH->AFR[1]  = 0xCCCCCCCC;
    /* Configure PHx pins in Alternate function mode */
    GPIOH->MODER   = 0xAAAAAFFF;
    /* Configure PHx pins speed to 50 MHz */
    GPIOH->OSPEEDR = 0xFFFFF000;
    /* Configure PHx pins Output type to push-pull */
    GPIOH->OTYPER  = 0x00000000;
    /* No pull-up, pull-down for PHx pins */
    GPIOH->PUPDR   = 0x55555000;
    /* Connect PIx pins to FMC Alternate function */
    GPIOI->AFR[0]  = 0xCCCCCCCC;
    GPIOI->AFR[1]  = 0x00000CC0;
    /* Configure PIx pins in Alternate function mode */
    GPIOI->MODER   = 0xFFEBAAAA;
    /* Configure PIx pins speed to 50 MHz */
    GPIOI->OSPEEDR = 0x003CFFFF;
    /* Configure PIx pins Output type to push-pull */
    GPIOI->OTYPER  = 0x00000000;
    /* No pull-up, pull-down for PIx pins */
    GPIOI->PUPDR   = 0x00145555;
/*-- FMC Configuration ------------------------------------------------------*/
    /* Enable the FMC interface clock */
    (RCC->AHB3ENR |= (RCC_AHB3ENR_FMCEN));
    /*SDRAM Timing and access interface configuration*/
    /*LoadToActiveDelay    = 2
      ExitSelfRefreshDelay = 10
      SelfRefreshTime      = 6
      RowCycleDelay        = 8
      WriteRecoveryTime    = 2
      RPDelay              = 2
      RCDDelay             = 2
      SDBank             = FMC_SDRAM_BANK2
      ColumnBitsNumber   = FMC_SDRAM_COLUMN_BITS_NUM_9
      RowBitsNumber      = FMC_SDRAM_ROW_BITS_NUM_13
      MemoryDataWidth    = FMC_SDRAM_MEM_BUS_WIDTH_32
      InternalBankNumber = FMC_SDRAM_INTERN_BANKS_NUM_4
      CASLatency         = FMC_SDRAM_CAS_LATENCY_2
      WriteProtection    = FMC_SDRAM_WRITE_PROTECTION_DISABLE
      SDClockPeriod      = FMC_SDRAM_CLOCK_PERIOD_2
      ReadBurst          = FMC_SDRAM_RBURST_ENABLE
      ReadPipeDelay      = FMC_SDRAM_RPIPE_DELAY_1*/

    FMC_Bank5_6_R->SDCR[0] = 0x00003800;
    FMC_Bank5_6_R->SDCR[1] = 0x00000169;
    FMC_Bank5_6_R->SDTR[0] = 0x00107000;
    FMC_Bank5_6_R->SDTR[1] = 0x01010591;

    /* SDRAM initialization sequence */
    /* Clock enable command */
    FMC_Bank5_6_R->SDCMR = 0x00000009;
    tmpreg = FMC_Bank5_6_R->SDSR & 0x00000020;
    while((tmpreg != 0) && (timeout-- > 0))
    {
      tmpreg = FMC_Bank5_6_R->SDSR & 0x00000020;
    }

    /* Delay */
    for (index = 0; index<1000; index++);

    /* PALL command */
    FMC_Bank5_6_R->SDCMR = 0x0000000A;
    timeout = 0xFFFF;
    while((tmpreg != 0) && (timeout-- > 0))
    {
      tmpreg = FMC_Bank5_6_R->SDSR & 0x00000020;
    }

    FMC_Bank5_6_R->SDCMR = 0x000000EB;
    timeout = 0xFFFF;
    while((tmpreg != 0) && (timeout-- > 0))
    {
      tmpreg = FMC_Bank5_6_R->SDSR & 0x00000020;
    }

    FMC_Bank5_6_R->SDCMR = 0x0004400C;
    timeout = 0xFFFF;
    while((tmpreg != 0) && (timeout-- > 0))
    {
      tmpreg = FMC_Bank5_6_R->SDSR & 0x00000020;
    }
    /* Set refresh count */
    tmpreg = FMC_Bank5_6_R->SDRTR;
    FMC_Bank5_6_R->SDRTR = (tmpreg | (0x00000395<<1));    // 64ms/8K * 120Mhz
    // FMC_Bank5_6_R->SDRTR = (tmpreg | (0x000003FB<<1));    // 64ms/8K * 133Mhz

    /* Disable write protection */
    tmpreg = FMC_Bank5_6_R->SDCR[1];
    FMC_Bank5_6_R->SDCR[1] = (tmpreg & 0xFFFFFDFF);

    /* SDRAM Bank2 remapped on FMC bank2 and still accessible at default mapping */
    FMC_Bank1_R->BTCR[0] |= 0x02000000;

    /*FMC controller Enable*/
    FMC_Bank1_R->BTCR[0] |= 0x80000000;

#endif /* DATA_IN_ExtSDRAM */

#if defined(DATA_IN_ExtSRAM)
/*-- GPIOs Configuration -----------------------------------------------------*/
      /* Enable GPIOD, GPIOE, GPIOF and GPIOG interface clock */
    RCC->AHB4ENR   |= 0x00000078;

    /* Connect PDx pins to FMC Alternate function */
    GPIOD->AFR[0]  = 0x00CCC0CC;
    GPIOD->AFR[1]  = 0xCCCCCCCC;
    /* Configure PDx pins in Alternate function mode */
    GPIOD->MODER   = 0xAAAA0A8A;
    /* Configure PDx pins speed to 100 MHz */
    GPIOD->OSPEEDR = 0xFFFF0FCF;
    /* Configure PDx pins Output type to push-pull */
    GPIOD->OTYPER  = 0x00000000;
    /* No pull-up, pull-down for PDx pins */
    GPIOD->PUPDR   = 0x55550545;

    /* Connect PEx pins to FMC Alternate function */
    GPIOE->AFR[0]  = 0xC00CC0CC;
    GPIOE->AFR[1]  = 0xCCCCCCCC;
    /* Configure PEx pins in Alternate function mode */
    GPIOE->MODER   = 0xAAAA828A;
    /* Configure PEx pins speed to 100 MHz */
    GPIOE->OSPEEDR = 0xFFFFC3CF;
    /* Configure PEx pins Output type to push-pull */
    GPIOE->OTYPER  = 0x00000000;
    /* No pull-up, pull-down for PEx pins */
    GPIOE->PUPDR   = 0x55554145;

    /* Connect PFx pins to FMC Alternate function */
    GPIOF->AFR[0]  = 0x00CCCCCC;
    GPIOF->AFR[1]  = 0xCCCC0000;
    /* Configure PFx pins in Alternate function mode */
    GPIOF->MODER   = 0xAA000AAA;
    /* Configure PFx pins speed to 100 MHz */
    GPIOF->OSPEEDR = 0xFF000FFF;
    /* Configure PFx pins Output type to push-pull */
    GPIOF->OTYPER  = 0x00000000;
    /* No pull-up, pull-down for PFx pins */
    GPIOF->PUPDR   = 0x55000555;

    /* Connect PGx pins to FMC Alternate function */
    GPIOG->AFR[0]  = 0x00CCCCCC;
    GPIOG->AFR[1]  = 0x000000C0;
    /* Configure PGx pins in Alternate function mode */
    GPIOG->MODER   = 0x00200AAA;
    /* Configure PGx pins speed to 100 MHz */
    GPIOG->OSPEEDR = 0x00300FFF;
    /* Configure PGx pins Output type to push-pull */
    GPIOG->OTYPER  = 0x00000000;
    /* No pull-up, pull-down for PGx pins */
    GPIOG->PUPDR   = 0x00100555;

/*-- FMC/FSMC Configuration --------------------------------------------------*/
    /* Enable the FMC/FSMC interface clock */
    (RCC->AHB3ENR |= (RCC_AHB3ENR_FMCEN));

    /* Configure and enable Bank1_SRAM2 */
    FMC_Bank1->BTCR[4]  = 0x00001091;
    FMC_Bank1->BTCR[5]  = 0x00110212;
    FMC_Bank1E->BWTR[4] = 0x0FFFFFFF;

    /*FMC controller Enable*/
    FMC_Bank1->BTCR[0]  |= 0x80000000;

#endif /* DATA_IN_ExtSRAM */
}
#endif /* DATA_IN_ExtSRAM || DATA_IN_ExtSDRAM */

/**
  * @brief  Exit Run* mode and Configure the system Power Supply
  *
  * @note   This function exits the Run* mode and configures the system power supply
  *         according to the definition to be used at compilation preprocessing level.
  *         The application shall set one of the following configuration option:
  *           - PWR_LDO_SUPPLY
  *           - PWR_DIRECT_SMPS_SUPPLY
  *           - PWR_EXTERNAL_SOURCE_SUPPLY
  *           - PWR_SMPS_1V8_SUPPLIES_LDO
  *           - PWR_SMPS_2V5_SUPPLIES_LDO
  *           - PWR_SMPS_1V8_SUPPLIES_EXT_AND_LDO
  *           - PWR_SMPS_2V5_SUPPLIES_EXT_AND_LDO
  *           - PWR_SMPS_1V8_SUPPLIES_EXT
  *           - PWR_SMPS_2V5_SUPPLIES_EXT
  *
  * @note   The function modifies the PWR->CR3 register to enable or disable specific
  *         power supply modes and waits until the voltage level flag is set, indicating
  *         that the power supply configuration is stable.
  *
  * @param  None
  * @retval None
  */
void ExitRun0Mode(void)
{
#if defined(USE_PWR_LDO_SUPPLY)
  #if defined(SMPS)
    /* Exit Run* mode by disabling SMPS and enabling LDO */
    PWR->CR3 = (PWR->CR3 & ~PWR_CR3_SMPSEN) | PWR_CR3_LDOEN;
  #else
    /* Enable LDO mode */
    PWR->CR3 |= PWR_CR3_LDOEN;
  #endif /* SMPS */
  /* Wait till voltage level flag is set */
  while ((PWR->CSR1 & PWR_CSR1_ACTVOSRDY) == 0U)
  {}
#elif defined(USE_PWR_EXTERNAL_SOURCE_SUPPLY)
  #if defined(SMPS)
    /* Exit Run* mode */
    PWR->CR3 = (PWR->CR3 & ~(PWR_CR3_SMPSEN | PWR_CR3_LDOEN)) | PWR_CR3_BYPASS;
  #else
    PWR->CR3 = (PWR->CR3 & ~(PWR_CR3_LDOEN)) | PWR_CR3_BYPASS;
  #endif /* SMPS */
  /* Wait till voltage level flag is set */
  while ((PWR->CSR1 & PWR_CSR1_ACTVOSRDY) == 0U)
  {}
#elif defined(USE_PWR_DIRECT_SMPS_SUPPLY) && defined(SMPS)
  /* Exit Run* mode */
  PWR->CR3 &= ~(PWR_CR3_LDOEN);
  /* Wait till voltage level flag is set */
  while ((PWR->CSR1 & PWR_CSR1_ACTVOSRDY) == 0U)
  {}
#elif defined(USE_PWR_SMPS_1V8_SUPPLIES_LDO) && defined(SMPS)
  /* Exit Run* mode */
  PWR->CR3 |= PWR_CR3_SMPSLEVEL_0 | PWR_CR3_SMPSEN | PWR_CR3_LDOEN;
  /* Wait till voltage level flag is set */
  while ((PWR->CSR1 & PWR_CSR1_ACTVOSRDY) == 0U)
  {}
#elif defined(USE_PWR_SMPS_2V5_SUPPLIES_LDO) && defined(SMPS)
  /* Exit Run* mode */
  PWR->CR3 |= PWR_CR3_SMPSLEVEL_1 | PWR_CR3_SMPSEN | PWR_CR3_LDOEN;
  /* Wait till voltage level flag is set */
  while ((PWR->CSR1 & PWR_CSR1_ACTVOSRDY) == 0U)
  {}
#elif defined(USE_PWR_SMPS_1V8_SUPPLIES_EXT_AND_LDO) && defined(SMPS)
  /* Exit Run* mode */
  PWR->CR3 |= PWR_CR3_SMPSLEVEL_0 | PWR_CR3_SMPSEXTHP | PWR_CR3_SMPSEN | PWR_CR3_LDOEN;
  /* Wait till voltage level flag is set */
  while ((PWR->CSR1 & PWR_CSR1_ACTVOSRDY) == 0U)
  {}
#elif defined(USE_PWR_SMPS_2V5_SUPPLIES_EXT_AND_LDO) && defined(SMPS)
  /* Exit Run* mode */
  PWR->CR3 |= PWR_CR3_SMPSLEVEL_1 | PWR_CR3_SMPSEXTHP | PWR_CR3_SMPSEN | PWR_CR3_LDOEN;
  /* Wait till voltage level flag is set */
  while ((PWR->CSR1 & PWR_CSR1_ACTVOSRDY) == 0U)
  {}
#elif defined(USE_PWR_SMPS_1V8_SUPPLIES_EXT) && defined(SMPS)
  /* Exit Run* mode */
  PWR->CR3 = (PWR->CR3 & ~(PWR_CR3_LDOEN)) | PWR_CR3_SMPSLEVEL_0 | PWR_CR3_SMPSEXTHP | PWR_CR3_SMPSEN | PWR_CR3_BYPASS;
  /* Wait till voltage level flag is set */
  while ((PWR->CSR1 & PWR_CSR1_ACTVOSRDY) == 0U)
  {}
#elif defined(USE_PWR_SMPS_2V5_SUPPLIES_EXT) && defined(SMPS)
  /* Exit Run* mode */
  PWR->CR3 = (PWR->CR3 & ~(PWR_CR3_LDOEN)) | PWR_CR3_SMPSLEVEL_1 | PWR_CR3_SMPSEXTHP | PWR_CR3_SMPSEN | PWR_CR3_BYPASS;
  /* Wait till voltage level flag is set */
  while ((PWR->CSR1 & PWR_CSR1_ACTVOSRDY) == 0U)
  {}
#else
  /* No system power supply configuration is selected at exit Run* mode */
#endif /* USE_PWR_LDO_SUPPLY */
}

/**
  * @}
  */

/**
  * @}
  */

/**
  * @}
  */
