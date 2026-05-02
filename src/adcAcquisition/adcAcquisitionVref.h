/**
 * Copyright (C) 2025 by Electronya
 *
 * @file      adcAcquisitionVref.h
 * @author    jbacon
 * @date      2025-09-27
 * @brief     ADC Acquisition VREFINT Platform Abstraction
 *
 *            Maps STM32 family differences into two uniform macros:
 *              STM32_ADC_VREF_REG  — register lvalue to write
 *              STM32_ADC_VREF_BIT  — enable bit mask
 *
 *            Both are defined only on STM32 targets (or when the caller
 *            provides mock definitions for unit tests). When neither applies,
 *            enableVrefint() compiles to a no-op.
 *
 *            An optional STM32_ADC_VREF_REG_READ macro controls the readback
 *            check after the write. It defaults to STM32_ADC_VREF_REG but may
 *            be overridden before inclusion to inject a test-time mock.
 *
 * @ingroup   adc-acquisition
 * @{
 */

#ifndef ADC_ACQUISITION_VREF_H
#define ADC_ACQUISITION_VREF_H

/* Only pull in STM32 hardware headers when building for real hardware. */
#if defined(CONFIG_SOC_FAMILY_STM32)
#include <soc.h>
#include <stm32_ll_adc.h>
#endif

/*
 * F1/F2/F4: VREFINT enable lives in ADC1->CR2. Symbols come from stm32_ll_adc.h
 * which is only available in a real STM32 build.
 */
#if defined(CONFIG_SOC_FAMILY_STM32) && \
    (defined(CONFIG_SOC_SERIES_STM32F1X) || defined(CONFIG_SOC_SERIES_STM32F2X) || \
     defined(CONFIG_SOC_SERIES_STM32F4X))
  #define STM32_ADC_VREF_REG    ADC1->CR2
  #define STM32_ADC_VREF_BIT    ADC_CR2_TSVREFE

/*
 * Single-ADC parts (F302/F301, G0, L4, …) and dual-ADC parts (F303, G474, …)
 * use a CCR register. ADC1_COMMON / ADC12_COMMON / ADC_COMMON are either
 * provided by stm32_ll_adc.h on hardware or by the unit-test mock, so no
 * CONFIG_SOC_FAMILY_STM32 guard is needed here.
 */
#elif defined(ADC1_COMMON) && defined(ADC_CCR_VREFEN)
  #define STM32_ADC_VREF_REG    ADC1_COMMON->CCR
  #define STM32_ADC_VREF_BIT    ADC_CCR_VREFEN
#elif defined(ADC12_COMMON) && defined(ADC_CCR_VREFEN)
  #define STM32_ADC_VREF_REG    ADC12_COMMON->CCR
  #define STM32_ADC_VREF_BIT    ADC_CCR_VREFEN
#elif defined(ADC_COMMON) && defined(ADC_CCR_VREFEN)
  #define STM32_ADC_VREF_REG    ADC_COMMON->CCR
  #define STM32_ADC_VREF_BIT    ADC_CCR_VREFEN
#elif defined(CONFIG_SOC_FAMILY_STM32)
  #error "adcAcquisitionVref.h: unsupported STM32 variant — add ADC common register mapping"
#endif /* register selection */

/*
 * STM32_ADC_VREF_REG_READ — the expression used to read the register back
 * after writing.  Defaults to STM32_ADC_VREF_REG (direct register read) but
 * may be overridden before this header is included to inject a mock that
 * simulates a hardware failure (e.g. unit tests).
 */
#if defined(STM32_ADC_VREF_REG) && !defined(STM32_ADC_VREF_REG_READ)
  #define STM32_ADC_VREF_REG_READ (STM32_ADC_VREF_REG)
#endif

#endif /* ADC_ACQUISITION_VREF_H */

/** @} */
