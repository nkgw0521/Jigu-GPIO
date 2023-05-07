/************************************************************************/
/*	MODULE:	   UART モジュール											*/
/************************************************************************/
/**
	@file		UART_Driver.h
	@brief		UART モジュール
	@author 	Yasuhisa Nakagawa

	$Revision:$
	$Date:$
*/

#ifndef    __UART_DRIVER_H__
#define    __UART_DRIVER_H__

/************************************************************************/
/* INCLUDE FILE SECTION 												*/
/************************************************************************/
/* nothing */

/************************************************************************/
/* MACRO DEFINITION SECTION 											*/
/************************************************************************/
#undef	USE_UART_COMMON_MODULE


/************************************************************************/
/* TYPEDEF DEFINITION SECTION											*/
/************************************************************************/

typedef enum {
	UART_BAUD_9600 = 9600,
	UART_BAUD_19200 = 19200,
	UART_BAUD_38400 = 38400,
	UART_BAUD_57600 = 57600,
	UART_BAUD_115200 = 115200,
	UART_BAUD_NUM
} ENUM_UART_BAUD ;

/************************************************************************/
/* GLOBAL DATA DEFINITION SECTION(extern)							   */
/************************************************************************/
/* nothing */

/************************************************************************/
/* FUNCTION PROTOTYPE DEFINITION SECTION(PUBLIC)						*/
/************************************************************************/
/* nothing */

/************************************************************************/
/* FUNCTION PROTOTYPE DEFINITION SECTION(static)						*/
/************************************************************************/
/* nothing */

/************************************************************************/
/* FUNCTION PROTOTYPE DEFINITION SECTION(extern)						*/
/************************************************************************/
extern
int
UART_Read( void ) ;
extern
int
UART_Write(uint8_t *pcBuf, uint16_t ulLen) ;
extern
void
UART_Init( UART_HandleTypeDef * handle ) ;
extern
void
UART_Terminate(void) ;
extern
void
UART_SetBaudRate(uint32_t baudRate) ;
extern
void
UART_EmergencyPrint( char * buf ) ;

#endif /*__UART_DRIVER_H__*/
