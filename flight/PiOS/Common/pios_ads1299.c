/**
 ******************************************************************************
 * @addtogroup PIOS PIOS Core hardware abstraction layer
 * @{
 * @addtogroup PIOS_ADS1299 ADS1299 Functions
 * @brief Deals with the hardware interface to the EEG ADC chip
 * @{
 *
 * @file       pios_ads1299.c
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

/* Project Includes */
#include "pios.h"

#if defined(PIOS_INCLUDE_ADS1299)
#include "pios_ads1299.h"

/* Global Variables */

enum pios_ads1299_dev_magic {
    PIOS_ADS1299_DEV_MAGIC = 0xaba9b3ed,
};

#define PIOS_ADS1299_MAX_QUEUESIZE 2

struct ads1299_dev {
	uint32_t spi_id;
	uint32_t slave_num;
	xQueueHandle queue;
	const struct pios_ads1299_cfg *cfg;
	volatile bool configured;
	enum pios_ads1299_dev_magic magic;
};

//! The packed data raw from the chip
struct ads1299_packed_data {
	uint8_t  status[3];
	uint8_t  channels[8*3];
};

//! Global structure for this device device
static struct ads1299_dev *pios_ads1299_dev;

//! Private functions
static struct ads1299_dev *PIOS_ADS1299_alloc(void);
static int32_t PIOS_ADS1299_Validate(struct ads1299_dev *dev);
static int32_t PIOS_ADS1299_ClaimBus();
static int32_t PIOS_ADS1299_ReleaseBus();
static int32_t PIOS_ADS1299_SendCommand(uint8_t command);
static int32_t PIOS_ADS1299_SetReg(uint8_t address, uint8_t buffer);
static int32_t PIOS_ADS1299_GetReg(uint8_t address);
static int32_t PIOS_ADS1299_ReadID();

//! Private definitions

// Opcodes
#define ADS1299_WAKEUP    0x02
#define ADS1299_STANDBY   0x04
#define ADS1299_RESET     0x06
#define ADS1299_START     0x08
#define ADS1299_STOP      0x0A
#define ADS1299_RDATAC    0x10
#define ADS1299_SDATAC    0x11
#define ADS1299_RDATA     0x12
#define ADS1299_RREG      0x20
#define ADS1299_WREG      0x40

// Registers
#define ADS1299_REG_ID 0x00
#define ADS1299_REG_CONFIG1 0x01  /* default ok */
#define ADS1299_REG_CONFIG2 0x02  /* default ok */
 
#define ADS1299_REG_CONFIG3 0x03
#define ADS1299_CONFIG3_PWR_REF 0x80 /* turn on internal reference buffer */
#define ADS1299_CONFIG3_BIAS_INT 0x08 /* turn on internal bias reference */
#define ADS1299_CONFIG3_PWR_BIAS 0x04 /* turn on bias buffer */
 
/* use the settings here to generate pulses for impedance measurements */
#define ADS1299_REG_LOFF 0x04
 
/* defaults ok */
#define ADS1299_REG_CH1SET 0x05
#define ADS1299_REG_CH2SET 0x06
#define ADS1299_REG_CH3SET 0x07
#define ADS1299_REG_CH4SET 0x08
#define ADS1299_REG_CH5SET 0x09
#define ADS1299_REG_CH6SET 0x0A
#define ADS1299_REG_CH7SET 0x0B
#define ADS1299_REG_CH8SET 0x0C
 
#define ADS1299_REG_BIAS_SENSP 0x0D /* set to 0xFF if all electrodes are connected */
#define ADS1299_REG_BIAS_SENSN 0x0E /* default ok (no electrodes connected) */
 
#define ADS1299_REG_LOFF_SENSP 0x0F
#define ADS1299_REG_LOFF_SENSN 0x10
#define ADS1299_REG_LOFF_FLIP 0x11
#define ADS1299_REG_LOFF_STATP 0x12
#define ADS1299_REG_LOFF_STATN 0x13
#define ADS1299_REG_GPIO 0x14

#define ADS1299_REG_MISC1 0x15
#define ADS1299_MISC1_SRB1 0x20 /* connect all negative inputs to SRB1 */

#define ADS1299_REG_MISC2 0x16
#define ADS1299_REG_CONFIG4 0x17

/**
 * @brief Allocate a new device
 */
static struct ads1299_dev *PIOS_ADS1299_alloc(void)
{
	struct ads1299_dev *ads1299_dev;

	ads1299_dev = (struct ads1299_dev *)PIOS_malloc(sizeof(*ads1299_dev));

	if (!ads1299_dev) return (NULL);

	ads1299_dev->magic = PIOS_ADS1299_DEV_MAGIC;

	ads1299_dev->configured = false;

	ads1299_dev->queue = xQueueCreate(PIOS_ADS1299_MAX_QUEUESIZE, sizeof(struct pios_ads1299_data));

	if (ads1299_dev->queue == NULL) {
		vPortFree(ads1299_dev);
		return NULL;
	}

	return ads1299_dev;
}

/**
 * @brief Validate the handle to the spi device
 * @returns 0 for valid device or -1 otherwise
 */
static int32_t PIOS_ADS1299_Validate(struct ads1299_dev *dev)
{
	if (dev == NULL)
		return -1;

	if (dev->magic != PIOS_ADS1299_DEV_MAGIC)
		return -2;

	if (dev->spi_id == 0)
		return -3;

	return 0;
}

/**
 * @brief Initialize the ADS1299 EEG
 * @return 0 for success, -1 for failure
 */
int32_t PIOS_ADS1299_Init(uint32_t spi_id, uint32_t slave_num, const struct pios_ads1299_cfg *cfg)
{
	pios_ads1299_dev = PIOS_ADS1299_alloc();

	if (pios_ads1299_dev == NULL)
		return -1;

	pios_ads1299_dev->spi_id = spi_id;
	pios_ads1299_dev->slave_num = slave_num;
	pios_ads1299_dev->cfg = cfg;

	// 1. Enable the power line
	// 2. Trigger a reset
	GPIO_Init(cfg->pwdn.gpio, (GPIO_InitTypeDef*)&cfg->pwdn.init);
	GPIO_Init(cfg->reset.gpio, (GPIO_InitTypeDef*)&cfg->reset.init);
	GPIO_Init(cfg->start.gpio, (GPIO_InitTypeDef*)&cfg->start.init);

	GPIO_ResetBits(cfg->start.gpio, cfg->start.init.GPIO_Pin);
	GPIO_ResetBits(cfg->reset.gpio, cfg->reset.init.GPIO_Pin);
	GPIO_ResetBits(cfg->pwdn.gpio, cfg->pwdn.init.GPIO_Pin);

	PIOS_DELAY_WaitmS(100);

	GPIO_SetBits(cfg->pwdn.gpio, cfg->pwdn.init.GPIO_Pin);
	GPIO_SetBits(cfg->reset.gpio, cfg->reset.init.GPIO_Pin);
	PIOS_DELAY_WaitmS(5);
	GPIO_ResetBits(cfg->reset.gpio, cfg->reset.init.GPIO_Pin);
	PIOS_DELAY_WaitmS(5);
	GPIO_SetBits(cfg->reset.gpio, cfg->reset.init.GPIO_Pin);

	PIOS_WDG_Clear();

	PIOS_DELAY_WaitmS(100);

	if (false) {
		PIOS_ADS1299_SetReg(0,0);
		PIOS_ADS1299_GetReg(0);
	}

	// The chip resets in continuous data mode which blocks reading registers
	PIOS_ADS1299_SendCommand(ADS1299_SDATAC);

	if ((PIOS_ADS1299_ReadID() & 0x1F) != 0b00011110)
		return -1;

	PIOS_ADS1299_SetReg(ADS1299_REG_CONFIG1, 0x96); /* No daisy chain, 250 sps */

	PIOS_ADS1299_SetReg(ADS1299_REG_CONFIG3, ADS1299_CONFIG3_PWR_REF | 
	                    ADS1299_CONFIG3_BIAS_INT | ADS1299_CONFIG3_PWR_BIAS);


	if (true) {
		// Connect all the negative inputs to SRB1
		PIOS_ADS1299_SetReg(ADS1299_REG_MISC1, ADS1299_MISC1_SRB1);
		for (uint32_t i = 0; i < 3; i++)
			PIOS_ADS1299_SetReg(ADS1299_REG_CH1SET + i , 0x60);  // normal electrode
		for (uint32_t i = 3; i < 8; i++)
			PIOS_ADS1299_SetReg(ADS1299_REG_CH1SET + i , 0x61);  // normal electrode

		PIOS_ADS1299_SetReg(ADS1299_REG_CONFIG2, 0xC0); // No test signal
	} else {  // test mode
		PIOS_ADS1299_SetReg(ADS1299_REG_CONFIG2, 0b11010000); // Internal test signal
		PIOS_ADS1299_SetReg(ADS1299_REG_CH1SET, 0b01100101);  // Ch 1 = test signal
		PIOS_ADS1299_SetReg(ADS1299_REG_CH2SET, 0b01100011);  // Ch 2 = supply measurements (analog)
		PIOS_ADS1299_SetReg(ADS1299_REG_CH3SET, 0b01100011);  // Ch 3 = supply measurements (digital)
		PIOS_ADS1299_SetReg(ADS1299_REG_CH4SET, 0b01100100);  // Ch 4 = temperature
	}

	PIOS_ADS1299_EnableImpedance(false);

	if (true) {
		// Enable bias circuitry
		PIOS_ADS1299_SetReg(ADS1299_REG_BIAS_SENSP, 0x07); // Measure bias from all leads
		//PIOS_ADS1299_SetReg(ADS1299_REG_BIAS_SENSN, 0xFF); // Measure bias from all leads
	}

	// Register the sensor queue so user space can fetch data when available
	PIOS_SENSORS_Register(PIOS_SENSOR_EEG, pios_ads1299_dev->queue);

	pios_ads1299_dev->configured = true;

	// Set up EXTI line
	PIOS_EXTI_Init(cfg->exti_cfg);

	// Enable continuous reading of data
	PIOS_ADS1299_SendCommand(ADS1299_WAKEUP);
	PIOS_ADS1299_SendCommand(ADS1299_RDATAC);

	// Assert the start line
	GPIO_SetBits(cfg->start.gpio, cfg->start.init.GPIO_Pin);

	return 0;
}

//! Enable the 6nA impedance monitoring pulse
int32_t PIOS_ADS1299_EnableImpedance(bool enable)
{
	if (enable) {
		// Ouptut 6nA square pulse at fDR / 4
		PIOS_ADS1299_SetReg(ADS1299_REG_LOFF, 0x03);
		PIOS_ADS1299_SetReg(ADS1299_REG_LOFF_SENSP, 0xFF); // Test all leads
		PIOS_ADS1299_SetReg(ADS1299_REG_LOFF_SENSN, 0x00);
	} else {
		PIOS_ADS1299_SetReg(ADS1299_REG_LOFF_SENSP, 0x00); // Test no leads
		PIOS_ADS1299_SetReg(ADS1299_REG_LOFF_SENSN, 0x00);
	}

	return 0;
}

int32_t PIOS_ADS1299_ReadData(struct pios_ads1299_data *data)
{
	if (PIOS_ADS1299_ClaimBus() != 0)
		return -1;

	// Get the data in a packed format
	struct ads1299_packed_data rec_buf;
	if (PIOS_SPI_TransferBlock(pios_ads1299_dev->spi_id, NULL, (uint8_t *) &rec_buf, sizeof(rec_buf), NULL) < 0) {
		PIOS_ADS1299_ReleaseBus();
		return -2;
	}

	PIOS_ADS1299_ReleaseBus();

	for (uint32_t i = 0; i < 8; i ++) {
		data->channels[i] = (rec_buf.channels[i*3] << 16) |
		                   (rec_buf.channels[i*3 +1] << 8) | 
		                   (rec_buf.channels[i*3 + 2]);
	}

	return 0;
}

/**
 * Send command to the ADS1299 chip
 */
static int32_t PIOS_ADS1299_SendCommand(uint8_t command)
{
	if (PIOS_ADS1299_ClaimBus() != 0)
		return -1;

	if (PIOS_SPI_TransferByte(pios_ads1299_dev->spi_id, command) != 0) {
		PIOS_ADS1299_ReleaseBus();
		return -2;
	}

	PIOS_ADS1299_ReleaseBus();
	return 0;
}

/**
 * @brief Claim the SPI bus for the accel communications and select this chip
 * @return 0 if successful, -1 for invalid device, -2 if unable to claim bus
 */
static int32_t PIOS_ADS1299_ClaimBus()
{
	if (PIOS_ADS1299_Validate(pios_ads1299_dev) != 0)
		return -1;

	if (PIOS_SPI_ClaimBus(pios_ads1299_dev->spi_id) != 0)
		return -2;

	PIOS_SPI_RC_PinSet(pios_ads1299_dev->spi_id, pios_ads1299_dev->slave_num, 0);
	return 0;
}

/**
 * @brief Claim the SPI bus for the accel communications and select this chip
 * \param[in] pointer which receives if a task has been woken
 * @return 0 if successful, -1 for invalid device, -2 if unable to claim bus
 */
static int32_t PIOS_ADS1299_ClaimBusISR(bool *woken)
{
	if (PIOS_ADS1299_Validate(pios_ads1299_dev) != 0)
		return -1;

	if (PIOS_SPI_ClaimBusISR(pios_ads1299_dev->spi_id, woken) != 0)
		return -2;

	PIOS_SPI_RC_PinSet(pios_ads1299_dev->spi_id, pios_ads1299_dev->slave_num, 0);
	return 0;
}

/**
 * @brief Release the SPI bus for the accel communications and end the transaction
 * @return 0 if successful
 */
static int32_t PIOS_ADS1299_ReleaseBus()
{
	if (PIOS_ADS1299_Validate(pios_ads1299_dev) != 0)
		return -1;

	PIOS_SPI_RC_PinSet(pios_ads1299_dev->spi_id, pios_ads1299_dev->slave_num, 1);

	return PIOS_SPI_ReleaseBus(pios_ads1299_dev->spi_id);
}

/**
 * @brief Release the SPI bus for the accel communications and end the transaction
 * \param[in] pointer which receives if a task has been woken
 * @return 0 if successful
 */
static int32_t PIOS_ADS1299_ReleaseBusISR(bool *woken)
{
	if (PIOS_ADS1299_Validate(pios_ads1299_dev) != 0)
		return -1;

	PIOS_SPI_RC_PinSet(pios_ads1299_dev->spi_id, pios_ads1299_dev->slave_num, 1);

	return PIOS_SPI_ReleaseBusISR(pios_ads1299_dev->spi_id, woken);
}

/**
 * @brief Read a register from MPU6000
 * @returns The register value or -1 if failure to get bus
 * @param reg[in] Register address to be read
 */
static int32_t PIOS_ADS1299_GetReg(uint8_t reg)
{
	uint8_t data;

	if (PIOS_ADS1299_ClaimBus() != 0)
		return -1;

	PIOS_SPI_TransferByte(pios_ads1299_dev->spi_id, (0x20 | reg)); // set the register address
	PIOS_SPI_TransferByte(pios_ads1299_dev->spi_id, 0);            // request one byte
	data = PIOS_SPI_TransferByte(pios_ads1299_dev->spi_id, 0);     // receive response

	PIOS_ADS1299_ReleaseBus();
	return data;
}

/**
 * @brief Writes one byte to the MPU6000
 * \param[in] reg Register address
 * \param[in] data Byte to write
 * \return 0 if operation was successful
 * \return -1 if unable to claim SPI bus
 * \return -2 if unable to claim i2c device
 */
static int32_t PIOS_ADS1299_SetReg(uint8_t reg, uint8_t data)
{
	if (PIOS_ADS1299_ClaimBus() != 0)
		return -1;

	PIOS_SPI_TransferByte(pios_ads1299_dev->spi_id, 0x40 | reg);  // set the register address and write mode
	PIOS_SPI_TransferByte(pios_ads1299_dev->spi_id, 0);           // write one byte
	PIOS_SPI_TransferByte(pios_ads1299_dev->spi_id, data);

	PIOS_ADS1299_ReleaseBus();

	return 0;
}

/**
 * Read the ADS1299 device ID
 * @return id or < 0 if unsuccessful
 */
static int32_t PIOS_ADS1299_ReadID()
{
	uint32_t id = PIOS_ADS1299_GetReg(0);

	return id;
}

/**
* @brief IRQ Handler.  Read all the data from onboard buffer
*/
bool PIOS_ADS1299_IRQHandler(void)
{

	// vref / (2^23 - 1) * 1000 mV / V / gain
	const float scale = 4.5f / (8388608 - 1) * 1000000 / 24;

	if (PIOS_ADS1299_Validate(pios_ads1299_dev) != 0 || pios_ads1299_dev->configured == false)
		return false;

	bool woken = false;

	if (PIOS_ADS1299_ClaimBusISR(&woken) != 0)
		return false;

	struct ads1299_packed_data rec_buf;
	if (PIOS_SPI_TransferBlock(pios_ads1299_dev->spi_id, NULL, (uint8_t *) &rec_buf, sizeof(rec_buf), NULL) < 0) {
		PIOS_ADS1299_ReleaseBusISR(&woken);
		return woken;
	}

	PIOS_ADS1299_ReleaseBusISR(&woken);

	// Unpack the data before pushing it out
	struct pios_ads1299_data data;
	for (uint32_t i = 0; i < 8; i ++) {
		int32_t raw_data = (rec_buf.channels[i*3] << 16) |
		                   (rec_buf.channels[i*3 +1] << 8) | 
		                   (rec_buf.channels[i*3 + 2]);

		// Sign extend the data while unpacking
      	if (raw_data & 0x00800000)
      		raw_data |= 0xFF000000;

		data.channels[i] = raw_data * scale;
	}


	portBASE_TYPE xHigherPriorityTaskWoken;
	xQueueSendToBackFromISR(pios_ads1299_dev->queue, (void *)&data, &xHigherPriorityTaskWoken);

	return (xHigherPriorityTaskWoken == pdTRUE) || woken;
}

#endif /* PIOS_INCLUDE_ADS1299 */

/**
 * @}
 * @}
 */
