/**
 * @file app.c
 * @author y.nakagawa
 * @brief 
 * @version 0.1
 * @date 2023-05-05
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include "system.h"
#include "scpi/scpi.h"
#include "scpi.h"
#include "../../App/Inc/gpio.h"
#include "uart_lib.h"

#define	BUF_LINE_MAX	SCPI_INPUT_BUFFER_LENGTH		/*受信文字列最大長 */

/**
 * コマンド受信バッファ
 */
static unsigned char gBufLine[ BUF_LINE_MAX + 2 ] ;

/**
 * @brief UART receive line buffer
 * 
 * @return int Receive buffer length.
 */
static int LineBufGet( void )
{
	char ch = '\0' ;
	int nBufCount = 0 ;

	for ( ; nBufCount < BUF_LINE_MAX ; )
	{
		ch = UART_Getc( );
		if ( (ch == '\r') || (ch == '\n') )
		{
#if	defined(ECHO_BACK)
			UART_Puts( "\r\n" ) ;
#endif	/* ECHO_BACK */
			break ;
		}
		if ( ch == '\b' )
		{
			if ( nBufCount > 0 )
			{
				--nBufCount ;
#if	defined(ECHO_BACK)
				UART_Puts( "\b \b" ) ;
#endif	/* ECHO_BACK */
			}
		}
		else
		{
			gBufLine[ nBufCount++ ] = (unsigned char)ch ;
#if	defined(ECHO_BACK)
			UART_Putc( ch ) ;
#endif	/* ECHO_BACK */
		}
	}
	gBufLine[ nBufCount++ ] = '\n' ;
	gBufLine[ nBufCount ] = '\0' ;
	return nBufCount ;
}


/**
 * @brief user main function
 * 
 */
extern void pwm_service(void);
void app_main(void)
{
	GPIO_Init( ) ;
	UART_Init( &huart2 ) ;
	GPIO_LedWrite( GPIO_HIGH ) ;

	SCPI_PwmReset();

	SCPI_Init(&scpi_context,
			scpi_commands,
			&scpi_interface,
			scpi_units_def,
			SCPI_IDN1, SCPI_IDN2, SCPI_IDN3, SCPI_IDN4,
			scpi_input_buffer, SCPI_INPUT_BUFFER_LENGTH,
			scpi_error_queue_data, SCPI_ERROR_QUEUE_SIZE);

	while(1)
	{
		if ( 0 < LineBufGet( ) )
		{
			SCPI_Input( &scpi_context, (char const *)gBufLine, strlen( (char *)gBufLine ) ) ;
		}
		pwm_service();
	}
}
