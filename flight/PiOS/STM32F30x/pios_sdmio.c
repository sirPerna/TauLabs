/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_SDMIO SDMIO Functions
 * @brief Hardware functions to deal with SDM-IO-UART sonar sensor
 * @{
 *
 * @file       pios_sdmio.c
 * @author     Tau Labs, http://taulabs.org, Copyright (C) 2013
 * @brief      SDMIO private functions header.
 * @see        The GNU Public License (GPL) Version 3
 *
 ******************************************************************************/
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

#include "pios.h"

#ifdef PIOS_INCLUDE_SDMIO
#include "pios_sdmio_priv.h"

/* Private constants */
#define SDMIO_TASK_PRIORITY (tskIDLE_PRIORITY + configMAX_PRIORITIES - 1) // max priority
#define SDMIO_TASK_STACK (512 / 4)
#define UPDATE_PERIOD_MS 100
/* Private methods */
static void PIOS_SDMIO_Task(void *parameters);

/* Local Variables */
/* 100 ms timeout without updates on channels */
const static uint32_t SDMIO_SUPERVISOR_TIMEOUT = 100000;

enum pios_sdmio_dev_magic {
    PIOS_SDMIO_DEV_MAGIC = 0xba2929bc,
};

#define PIOS_SDMIO_NUM_CHANNELS 1

struct pios_sdmio_dev {
	const struct pios_sdmio_cfg *cfg;
	xTaskHandle task;
	xQueueHandle queue;

	uint8_t CaptureState[PIOS_SDMIO_NUM_CHANNELS];
	uint16_t RiseValue[PIOS_SDMIO_NUM_CHANNELS];
	uint16_t FallValue[PIOS_SDMIO_NUM_CHANNELS];
	uint16_t TriggerValue[PIOS_SDMIO_NUM_CHANNELS];
	uint32_t CaptureValue[PIOS_SDMIO_NUM_CHANNELS];
	uint32_t overflow;
	enum pios_sdmio_dev_magic magic;
};

static struct pios_sdmio_dev *dev;

/**
 * @brief Allocate a new device
 */
static struct pios_sdmio_dev *PIOS_SDMIO_alloc(void)
{
	struct pios_sdmio_dev *sdmio_dev;

	sdmio_dev = (struct pios_sdmio_dev *)pvPortMalloc(sizeof(*sdmio_dev));
	if (!sdmio_dev) {
		return NULL;
	}

	sdmio_dev->queue = xQueueCreate(1, sizeof(struct pios_sensor_sonar_data));
	if (sdmio_dev->queue == NULL) {
		vPortFree(sdmio_dev);
		return NULL;
	}

	sdmio_dev->magic = PIOS_SDMIO_DEV_MAGIC;
	return sdmio_dev;
}

/**
 * @brief Validate the handle to the device
 * @returns 0 for valid device or <0 otherwise
 */
static bool PIOS_SDMIO_validate(struct pios_sdmio_dev *dev)
{
	if (dev == NULL)
		return -1;
	if (dev->magic != PIOS_SDMIO_DEV_MAGIC)
		return -2;
	return 0;
}

static void PIOS_SDMIO_tim_overflow_cb(uintptr_t id, uintptr_t context, uint8_t channel, uint16_t count);
static void PIOS_SDMIO_tim_edge_cb(uintptr_t id, uintptr_t context, uint8_t channel, uint16_t count);
const static struct pios_tim_callbacks tim_callbacks = {
	.overflow = PIOS_SDMIO_tim_overflow_cb,
	.edge     = PIOS_SDMIO_tim_edge_cb,
};

uint32_t edge_counts = 0;

/**
 * Initialises all the pins
 */
int32_t PIOS_SDMIO_Init(uintptr_t *sdmio_id, const struct pios_sdmio_cfg *cfg)
{
	PIOS_DEBUG_Assert(sdmio_id);
	PIOS_DEBUG_Assert(cfg);

	struct pios_sdmio_dev *sdmio_dev;

	sdmio_dev = (struct pios_sdmio_dev *)PIOS_SDMIO_alloc();
	if (!sdmio_dev) {
		return -1;
	}

	if (PIOS_SDMIO_validate(sdmio_dev) != 0) {
		return -1;
	}

	/* Bind the configuration to the device instance */
	sdmio_dev->cfg = cfg;
	dev  = sdmio_dev;

	uintptr_t tim_id;
	if (PIOS_TIM_InitChannels(&tim_id, cfg->channels, cfg->num_channels, &tim_callbacks, (uintptr_t)sdmio_dev)) {
		return -1;
	}
	if (tim_id == 0) {
		return -1;
	}

	/* Configure the channels to be in capture/compare mode */

	const struct pios_tim_channel *chan   = &cfg->channels[0];

	/* Configure timer for input capture */
	TIM_ICInitTypeDef TIM_ICInitStructure = cfg->tim_ic_init;
	TIM_ICInitStructure.TIM_Channel = chan->timer_chan;
	TIM_ICInit(chan->timer, &TIM_ICInitStructure);

	/* Enable the Capture Compare Interrupt Request */
	switch (chan->timer_chan) {
	case TIM_Channel_1:
		TIM_ITConfig(chan->timer, TIM_IT_CC1, ENABLE);
		break;
	case TIM_Channel_2:
		TIM_ITConfig(chan->timer, TIM_IT_CC2, ENABLE);
		break;
	case TIM_Channel_3:
		TIM_ITConfig(chan->timer, TIM_IT_CC3, ENABLE);
		break;
	case TIM_Channel_4:
		TIM_ITConfig(chan->timer, TIM_IT_CC4, ENABLE);
		break;
	}

	// Need the update event for that timer to detect timeouts
	TIM_ITConfig(chan->timer, TIM_IT_Update, ENABLE);


	GPIO_Init(sdmio_dev->cfg->trigger.gpio, (GPIO_InitTypeDef *)&sdmio_dev->cfg->trigger.init);

	*sdmio_id = (uintptr_t)sdmio_dev;

	portBASE_TYPE result = xTaskCreate(PIOS_SDMIO_Task, (const signed char *)"pios_sdmio",
	                                   SDMIO_TASK_STACK, NULL, SDMIO_TASK_PRIORITY,
	                                   &sdmio_dev->task);
	PIOS_Assert(result == pdPASS);

	PIOS_SENSORS_Register(PIOS_SENSOR_SONAR, sdmio_dev->queue);

	edge_counts++;

	return 0;
}

/**
 * Pulse the trigger line and store the time this was performed
 * because this sensor measures time as echo minus trigger time
 * as opposed to others that measure pulse duration
 */
static void PIOS_SDMIO_Trigger(void)
{
	// Set FSM to capture positive pulse duration
	dev->CaptureValue[0] = 0;
	dev->RiseValue[0] = 0;
	dev->FallValue[0] = 0;
	dev->overflow = 0;

	/* Switch polarity of input capture */
	dev->CaptureState[0] = 1;
	TIM_ICInitTypeDef TIM_ICInitStructure = dev->cfg->tim_ic_init;
	const struct pios_tim_channel *chan = &dev->cfg->channels[0];
	TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Falling;
	TIM_ICInitStructure.TIM_Channel    = chan->timer_chan;
	TIM_ICInit(chan->timer, &TIM_ICInitStructure);

	GPIO_SetBits(dev->cfg->trigger.gpio, dev->cfg->trigger.init.GPIO_Pin);
	dev->TriggerValue[0] = chan->timer->CNT;
	PIOS_DELAY_WaituS(15);
	GPIO_ResetBits(dev->cfg->trigger.gpio, dev->cfg->trigger.init.GPIO_Pin);
}

//! Valid pulse for SDM-IO-UART is 150 us
static int32_t PIOS_SDMIO_Completed(void)
{
	return (dev->CaptureValue[0] > 100 && dev->CaptureValue[0] < 200);
}

/**
 * Get the value of an input channel
 * \param[in] Channel Number of the channel desired
 * \output -1 Channel not available
 * \output  0 No echo received
 * \output >0 Channel value
 */
static int32_t PIOS_SDMIO_Get(void)
{
	/* http://www.elecfreaks.com/264.html                */
	/* pulse should be about 150 us for valid pulses and */
	/* 10 ms when no valid echo was detected             */
	if (PIOS_SDMIO_Completed())
		return ((uint16_t) (dev->FallValue[0] - dev->TriggerValue[0]));
	else return 0;
}

static void PIOS_SDMIO_tim_overflow_cb(uintptr_t tim_id, uintptr_t context, uint8_t channel, uint16_t count)
{
	edge_counts++;

	struct pios_sdmio_dev *sdmio_dev = (struct pios_sdmio_dev *)context;

	if (PIOS_SDMIO_validate(dev) != 0) {
		/* Invalid device specified */
		return;
	}

	if (channel >= sdmio_dev->cfg->num_channels) {
		/* Channel out of range */
		return;
	}

	sdmio_dev->overflow += count;
}


//! Capture the duration of the low pulse which should be 150 us for valid signal
static void PIOS_SDMIO_tim_edge_cb(uintptr_t tim_id, uintptr_t context, uint8_t chan_idx, uint16_t count)
{
	edge_counts++;
	/* Recover our device context */
	struct pios_sdmio_dev *sdmio_dev = (struct pios_sdmio_dev *)context;

	if (PIOS_SDMIO_validate(sdmio_dev) != 0) {
		/* Invalid device specified */
		return;
	}

	if (chan_idx >= sdmio_dev->cfg->num_channels) {
		/* Channel out of range */
		return;
	}

	const struct pios_tim_channel *chan = &sdmio_dev->cfg->channels[chan_idx];

	if (sdmio_dev->CaptureState[chan_idx] == 0) {
		sdmio_dev->RiseValue[chan_idx] = count;
	} else {
		sdmio_dev->FallValue[chan_idx] = count;
	}

	// flip state machine and capture value here
	/* Simple rise or fall state machine */
	TIM_ICInitTypeDef TIM_ICInitStructure = sdmio_dev->cfg->tim_ic_init;
	if (sdmio_dev->CaptureState[chan_idx] == 0) {
		/* Capture computation */
		if (sdmio_dev->RiseValue[chan_idx] > sdmio_dev->FallValue[chan_idx]) {
			sdmio_dev->CaptureValue[chan_idx] = (sdmio_dev->RiseValue[chan_idx] - sdmio_dev->FallValue[chan_idx]);
		} else {
			sdmio_dev->CaptureValue[chan_idx] = ((chan->timer->ARR - sdmio_dev->FallValue[chan_idx]) + sdmio_dev->RiseValue[chan_idx]);
		}

		/* Switch states */
		sdmio_dev->CaptureState[chan_idx] = 1;

		/* Switch polarity of input capture */
		TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Falling;
		TIM_ICInitStructure.TIM_Channel    = chan->timer_chan;
		TIM_ICInit(chan->timer, &TIM_ICInitStructure);
	} else {
		/* Switch states */
		sdmio_dev->CaptureState[chan_idx] = 0;

		/* Switch polarity of input capture */
		TIM_ICInitStructure.TIM_ICPolarity = TIM_ICPolarity_Rising;
		TIM_ICInitStructure.TIM_Channel    = chan->timer_chan;
		TIM_ICInit(chan->timer, &TIM_ICInitStructure);
	}
}

static void PIOS_SDMIO_Task(void *parameters)
{
	int32_t delay       = 0;
	float   distance    = 0;

	while (1) {
		struct pios_sensor_sonar_data data;
		// Compute the current ranging distance
		if (PIOS_SDMIO_Completed()) {
			delay = PIOS_SDMIO_Get();
			// from 2.55cm to 1.5m
			if (((delay > 150) && (delay < 12750))) {
				distance = (delay - 250) * (340.0e-6f / 2);

				data.range = distance;
				data.valid_range = true;
				xQueueSend(dev->queue, (void *)&data, 0);
			} else {
				if (delay <= 150)
					data.range = -1;
				data.valid_range = false;
				xQueueSend(dev->queue, (void *)&data, 0);
			}
		} else {
			// No response. Indicate no data.
			data.valid_range = false;
			xQueueSend(dev->queue, (void *)&data, 0);
		} 

		PIOS_SDMIO_Trigger();

		vTaskDelay(UPDATE_PERIOD_MS / portTICK_RATE_MS);
	}
}

#endif /* PIOS_INCLUDE_SDMIO */
