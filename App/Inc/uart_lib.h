/************************************************************************/
/* MODULE:	   UART ライブラリ モジュール		*/
/************************************************************************/
/**
	@file		UART_Lib.h
	@brief		UART ライブラリ モジュール
	@author 	y.nakagawa

	$Id:$
*/

#ifndef	__UART_LIB_H__
#define	__UART_LIB_H__

/************************************************************************/
/* INCLUDE FILE SECTION 												*/
/************************************************************************/
#include	"uart.h"

/************************************************************************/
/* MACRO DEFINITION SECTION 											*/
/************************************************************************/
#define	UART_CRLF	MODEL_COMMAND_CRLF


/************************************************************************/
/* TYPEDEF DEFINITION SECTION											*/
/************************************************************************/
/* nothing */

/************************************************************************/
/* GLOBAL DATA DEFINITION SECTION(extern)								*/
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
char
UART_Getc(void) ;

extern
void
UART_Put( char * buf, int len ) ;

extern
void
UART_Puts( char* buf ) ;

extern
void
UART_Putc( char ch ) ;

#endif	/* __UART_LIB_H__ */