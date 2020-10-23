/*
 * FreeRTOS+IO V1.0.1 (C) 2012 Real Time Engineers ltd.
 *
 * FreeRTOS+IO is an add-on component to FreeRTOS.  It is not, in itself, part
 * of the FreeRTOS kernel.  FreeRTOS+IO is licensed separately from FreeRTOS,
 * and uses a different license to FreeRTOS.  FreeRTOS+IO uses a dual license
 * model, information on which is provided below:
 *
 * - Open source licensing -
 * FreeRTOS+IO is a free download and may be used, modified and distributed
 * without charge provided the user adheres to version two of the GNU General
 * Public license (GPL) and does not remove the copyright notice or this text.
 * The GPL V2 text is available on the gnu.org web site, and on the following
 * URL: http://www.FreeRTOS.org/gpl-2.0.txt
 *
 * - Commercial licensing -
 * Businesses and individuals who wish to incorporate FreeRTOS+IO into
 * proprietary software for redistribution in any form must first obtain a low
 * cost commercial license - and in-so-doing support the maintenance, support
 * and further development of the FreeRTOS+IO product.  Commercial licenses can
 * be obtained from http://shop.freertos.org and do not require any source files
 * to be changed.
 *
 * FreeRTOS+IO is distributed in the hope that it will be useful.  You cannot
 * use FreeRTOS+IO unless you agree that you use the software 'as is'.
 * FreeRTOS+IO is provided WITHOUT ANY WARRANTY; without even the implied
 * warranties of NON-INFRINGEMENT, MERCHANTABILITY or FITNESS FOR A PARTICULAR
 * PURPOSE. Real Time Engineers Ltd. disclaims all conditions and terms, be they
 * implied, expressed, or statutory.
 *
 * 1 tab == 4 spaces!
 *
 * http://www.FreeRTOS.org
 * http://www.FreeRTOS.org/FreeRTOS-Plus
 *
 */

#ifndef TIVA_BASE_BOARD_H
#define TIVA_BASE_BOARD_H



/* Header files for all the driver libraries that can be used with this BSP. */
//#include "lpc17xx_uart.h"
//#include "lpc17xx_ssp.h"
//#include "lpc17xx_i2c.h"
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "inc/hw_ints.h"
#include "inc/hw_memmap.h"
#include "inc/hw_types.h"
#include "inc/hw_uart.h"
#include "driverlib/rom.h"
#include "driverlib/rom_map.h"
#include "driverlib/pin_map.h"
#include "driverlib/sysctl.h"
#include "driverlib/interrupt.h"
#include "driverlib/gpio.h"
#include "driverlib/uart.h"

/*******************************************************************************
 * Definitions used by FreeRTOS+IO to determine the peripherals that are
 * available on the board, and the functions used to interface with the target
 * specific peripheral drivers.
 ******************************************************************************/

/*******************************************************************************
 * Definitions used by the UART-interrupt-driven-command-console.c example file.
 *
 * See http://www.freertos.org/FreeRTOS-Plus/FreeRTOS_Plus_IO/Board_Support_Packages.shtml
 *
 ******************************************************************************/
#define boardAVAILABLE_DEVICES_LIST												\
{																				\
	{ ( const int8_t * const ) "/UART0/", eUART_TYPE, ( void * ) UART0_BASE }	\
}

/*******************************************************************************
 * Map the FreeRTOS+IO interface to the LPC17xx specific functions.
 ******************************************************************************/
portBASE_TYPE vFreeRTOS_Tiva_PopulateFunctionPointers( const Peripheral_Types_t ePeripheralType, Peripheral_Control_t * const pxPeripheralControl );
#define boardFreeRTOS_PopulateFunctionPointers vFreeRTOS_Tiva_PopulateFunctionPointers

/*******************************************************************************
 * These define the number of peripherals available on the microcontroller -
 * not the number of peripherals that are supported by the software
 ******************************************************************************/
#define boardNUM_SSPS				2 /* SSP0 to SSP1. */
#define boardNUM_UARTS				4 /* UART0 to UART3. */
#define boardNUM_I2CS				3 /* I2C0 to I2C2. */

/*******************************************************************************
 * Set the default baud rate used by a UART.
 ******************************************************************************/
#define boardDEFAULT_UART_BAUD		115200

/*******************************************************************************
 * Command console definitions.
 ******************************************************************************/
#define boardCOMMAND_CONSOLE_UART	( const int8_t * const ) "/UART0/"

#if 0
/*******************************************************************************
 * Configure port UART port pins to be correct for the wiring of the
 * LPCXpresso base board.
 ******************************************************************************/
#define boardCONFIGURE_UART_PINS( cPeripheralNumber, xPinConfig )					\
	switch( ( cPeripheralNumber ) )													\
	{																				\
		case 3	:	( xPinConfig ).Funcnum = 2;										\
					( xPinConfig ).Pinnum = 0;										\
					( xPinConfig ).Portnum = 0;										\
					PINSEL_ConfigPin( &( xPinConfig ) );							\
					( xPinConfig ).Pinnum = 1;										\
					PINSEL_ConfigPin( &( xPinConfig ) );							\
					break;															\
																					\
		default	:	/* These are either not implemented yet, or not available		\
					on this board.  Force an assert failure. */						\
					configASSERT( ( cPeripheralNumber ) - ( cPeripheralNumber ) );	\
					break;															\
	}

/*******************************************************************************
 * Configure port SSP port pins to be correct for the wiring of the
 * LPCXpresso base board.
 ******************************************************************************/
#define boardCONFIGURE_SSP_PINS( cPeripheralNumber, xPinConfig )					\
	switch( ( cPeripheralNumber ) )													\
	{																				\
		case 1	:	( xPinConfig ).Funcnum = 2;										\
					( xPinConfig ).OpenDrain = 0;									\
					( xPinConfig ).Pinmode = 0;										\
					( xPinConfig ).Portnum = 0;										\
					( xPinConfig ).Pinnum = 7;										\
					PINSEL_ConfigPin( &( xPinConfig ) );							\
					( xPinConfig ).Pinnum = 8;										\
					PINSEL_ConfigPin( &( xPinConfig ) );							\
					( xPinConfig ).Pinnum = 9;										\
					PINSEL_ConfigPin( &( xPinConfig ) );							\
					( xPinConfig ).Funcnum = 0;										\
					( xPinConfig ).Portnum = 2;										\
					( xPinConfig ).Pinnum = 2;										\
					PINSEL_ConfigPin( &( xPinConfig ) );							\
					break;															\
																					\
		default	:	/* These are either not implemented yet, or not available		\
					on this board.  Force an assert failure. */						\
					configASSERT( ( cPeripheralNumber ) - ( cPeripheralNumber ) );	\
					break;															\
	}

/*******************************************************************************
 * Configure port ENET port pins to be correct for the wiring of the
 * LPCXpresso base board.
 ******************************************************************************/
#define boardCONFIGURE_ENET_PINS( xPinConfig )		\
	( xPinConfig ).Funcnum = 1;						\
	( xPinConfig ).OpenDrain = 0;					\
	( xPinConfig ).Pinmode = 0;						\
	( xPinConfig ).Portnum = 1;						\
	( xPinConfig ).Pinnum = 0;						\
	PINSEL_ConfigPin( &( xPinConfig ) );			\
	( xPinConfig ).Pinnum = 1;						\
	PINSEL_ConfigPin( &( xPinConfig ) );			\
	( xPinConfig ).Pinnum = 4;						\
	PINSEL_ConfigPin( &( xPinConfig ) );			\
	( xPinConfig ).Pinnum = 8;						\
	PINSEL_ConfigPin( &( xPinConfig ) );			\
	( xPinConfig ).Pinnum = 9;						\
	PINSEL_ConfigPin( &( xPinConfig ) );			\
	( xPinConfig ).Pinnum = 10;						\
	PINSEL_ConfigPin( &( xPinConfig ) );			\
	( xPinConfig ).Pinnum = 14;						\
	PINSEL_ConfigPin( &( xPinConfig ) );			\
	( xPinConfig ).Pinnum = 15;						\
	PINSEL_ConfigPin( &( xPinConfig ) );			\
	( xPinConfig ).Pinnum = 16;						\
	PINSEL_ConfigPin( &( xPinConfig ) );			\
	( xPinConfig ).Pinnum = 17;						\
	PINSEL_ConfigPin( &( xPinConfig ) )

/*******************************************************************************
 * Configure port I2C port pins to be correct for the wiring of the
 * LPCXpresso base board.
 ******************************************************************************/
#define boardCONFIGURE_I2C_PINS( cPeripheralNumber, xPinConfig )					\
	switch( ( cPeripheralNumber ) )													\
	{																				\
		case 2	:	( xPinConfig ).Funcnum = 2;										\
					( xPinConfig ).Pinnum = 10;										\
					( xPinConfig ).Portnum = 0;										\
					PINSEL_ConfigPin( &( xPinConfig ) );							\
					( xPinConfig ).Pinnum = 11;										\
					PINSEL_ConfigPin( &( xPinConfig ) );							\
					break;															\
																					\
		default	:	/* These are either not implemented yet, or not available		\
					on this board.  Force an assert failure. */						\
					configASSERT( ( cPeripheralNumber ) - ( cPeripheralNumber ) );	\
					break;															\
	}



/*******************************************************************************
 * GPIO/LED polarities
 ******************************************************************************/
#define boardGPIO_OUTPUT			( 1 )
#define boardGPIO_INPUT				( 0 )
#define boardLED_ON					( 1 )
#define boardLED_OFF				( 0 )


/*******************************************************************************
 * PHY ID
 *
 * NOTE!!! This is here for information only.  The ID is hard coded into the
 * NXP driver, so changing this has no effect.  Search for the text boardPHY_ID
 * in lpc17xx_emac.c to see where the value is coded.
 ******************************************************************************/
#define boardPHY_ID 0x0007C0F0UL /* LAN8720_ID */

/*******************************************************************************
 * 7-Segment display wiring.
 ******************************************************************************/
#define board7SEGMENT_SSP_PORT						( const int8_t * const ) "/SSP1/"
#define board7SEG_CS_PORT							( 2 )
#define boardOLED_CS_PORT							( 0 )
#define board7SEG_CS_PIN							( 1 << 2 )
#define boardOLED_CS_PIN							( 1 << 6 )
#define board7SEG_DEASSERT_CS()						GPIO_SetValue( board7SEG_CS_PORT, board7SEG_CS_PIN )
#define boardOLED_DEASSERT_CS() 					GPIO_SetValue( boardOLED_CS_PORT, boardOLED_CS_PIN )
#define board7SEG_ASSERT_CS()						GPIO_ClearValue( board7SEG_CS_PORT, board7SEG_CS_PIN )

/*******************************************************************************
 * SD card wiring.
 ******************************************************************************/
#define boardSD_CARD_CS_PORT						( 2 )
#define boardSD_CARD_CS_PIN							( 1 << 2 )
#define boardSD_CARD_DETECT_PIN						( 1 << 11 )
#define boardSD_CARD_DETECT_PORT					( 2 )
#define boardSD_CARD_DETECT()						( GPIO_ReadValue( boardSD_CARD_DETECT_PORT ) & boardSD_CARD_DETECT_PIN )
#define boardSD_CARD_INSERTED						( 0 )
#define boardSD_CARD_DEASSERT_CS()					GPIO_SetValue( boardSD_CARD_CS_PORT, boardSD_CARD_CS_PIN )
#define boardSD_CARD_ASSERT_CS()					GPIO_ClearValue( boardSD_CARD_CS_PORT, boardSD_CARD_CS_PIN )
#define boardSD_CARD_SSP_PORT						( const int8_t * const ) "/SSP1/"

#define boardCONFIGURE_SD_CARD_CS_PINS()												\
{																						\
	/* Configure the IO pin used for the SD card detect line. */						\
	GPIO_SetDir( boardSD_CARD_CS_PORT, boardSD_CARD_DETECT_PIN, boardGPIO_INPUT );		\
																						\
	/* Configure the IO pin used for the SD card CS line. */							\
	GPIO_SetDir( boardSD_CARD_CS_PORT, boardSD_CARD_CS_PIN, boardGPIO_OUTPUT );			\
}

/*******************************************************************************
 * SSP/SPI port configuration definitions.
 ******************************************************************************/
#define boardSSP_DATABIT_8 							( SSP_DATABIT_8 )
#define boardSPI_SAMPLE_ON_LEADING_EDGE_CPHA_0 		( SSP_CPHA_FIRST )
#define boardSPI_SAMPLE_ON_TRAILING_EDGE_CPHA_1 	( SSPCPHA_SECOND )
#define boardSPI_CLOCK_BASE_VALUE_CPOL_1 			( SSP_CPOL_LO ) /* Looks strange, but is in accordance with the comments in the NXP code, which even says in the code it is not a bug so must get questioned often. */
#define boardSPI_CLOCK_BASE_VALUE_CPOL_0			( SSP_CPOL_HI ) /* Looks strange, but is in accordance with the comments in the NXP code, which even says in the code it is not a bug so must get questioned often. */
#define boardSPI_MASTER_MODE 						( SSP_MASTER_MODE )
#define boardSSP_FRAME_SPI 							( SSP_FRAME_SPI )
#define boardDEFAULT_READ_MUTEX_TIMEOUT				( ( portTickType ) 1000 )
#define boardSD_INTERFACE_FAST_CLOCK				10000000UL

/*******************************************************************************
 * OLED I2C port connections/configuration
 ******************************************************************************/
#define boardDEFAULT_I2C_SPEED					( 100000UL )
#define boardOLED_I2C_PORT						( const int8_t * const ) "/I2C2/"
#define boardI2C_MAX_RETRANSMISSIONS			( 3UL )

/*******************************************************************************
 * Definitions used if the generic software timer LED code is included in the build.
 ******************************************************************************/
#define boardSOFTWARE_TIMER_LED_PORT_INITIALISER	{ 2, 0 } /* The third LED output is also on port 2. */
#define boardSOFTWARE_TIMER_LED_INITIALISER			{ 1, ( 1 << 26 ) } /* The third LED output is on bit ( 1 << 1 ). */
#define boardNUM_SOFTWARE_TIMER_LEDS				( 2 ) /* The third is not used as it conflicts with the OLED power. */

/* SSP specific ioctl requests. */
#define ioctlSET_SSP_FRAME_FORMAT			1000

#endif

#endif /* LPCXPRESSO_BASE_BOARD_H */




