/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_ADS1299 ADS1299 Functions
 * @brief Deals with the hardware interface to the EEG ADC chip
 * @{
 *
 * @file       pios_ads1299.h
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2014
 * @brief      ADS1299 Interface library
 * @see        The GNU Public License (GPL) Version 3
 *
 ******************************************************************************
 */
/*
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 */

#ifndef PIOS_ADS1299_H
#define PIOS_ADS1299_H

#include "pios.h"

struct pios_ads1299_cfg {
	const struct pios_exti_cfg *exti_cfg; /* Pointer to the EXTI configuration */
	struct stm32_gpio pwdn;
	struct stm32_gpio reset;
	struct stm32_gpio start;	
};

/* Public Functions */
extern int32_t PIOS_ADS1299_Init(uint32_t spi_id, uint32_t slave_num, const struct pios_ads1299_cfg *new_cfg);
extern bool PIOS_ADS1299_IRQHandler(void);

#endif /* PIOS_ADS1299_H */

/** 
  * @}
  * @}
  */
