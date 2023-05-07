/************************************************************************/
/* MODULE:	   UART モジュール											*/
/************************************************************************/
/**
	@file		UART_Driver.c
	@brief		UART モジュール
	@author 	Yasuhisa Nakagawa

	$Revision:$
	$Date:$
*/

/************************************************************************/
/* INCLUDE FILE SECTION													*/
/************************************************************************/
#include "system.h"
#include "uart.h"

/************************************************************************/
/* MACRO DEFINITION SECTION												*/
/************************************************************************/
#define UART_RX_BUF_SIZE	(256)		/* UARTの受信バッファサイズ */

/************************************************************************/
/* TYPEDEF DEFINITION SECTION											*/
/************************************************************************/
/* nothing */

/************************************************************************/
/* GLOBAL DATA DEFINITION SECTION(PUBLIC)							   */
/************************************************************************/
/* nothing */

/************************************************************************/
/* GLOBAL DATA DEFINITION SECTION(static)								*/
/************************************************************************/
/* Peripheral handle */
static
UART_HandleTypeDef *	hUartHandle = NULL ;
/* 受信バッファ管理用 */
static uint8_t UART_rxBuf[UART_RX_BUF_SIZE];	/* 受信バッファ（DMAが使用） */
static int32_t UART_rxRp =  0 ;				/* 受信バッファリードポインタ */
static uint32_t UART_Error = 0 ;

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
static
void
UART_Reset( void ) ;

/************************************************************************/
/* FUNCTION PROTOTYPE DEFINITION SECTION(extern)						*/
/************************************************************************/
/* nothing */



/********************************************************************************/
/**
	@brief	UART_Reset 関数

	@author	y.nakagawa
	@date	2017/11/08(水) 10:17:26

	@return	なし (none)
*/
/********************************************************************************/
static
void
UART_Reset( void )
{
	UART_Error = 0 ;
	UART_rxRp =  0 ;
	/* UART+DMA受信 */
#if 1
	HAL_UART_Receive_DMA( hUartHandle, UART_rxBuf, UART_RX_BUF_SIZE);
#else
	/* UARTのフレーミングエラーに対しては最低限以下を行えば復帰する。 */
	/* HAL_UART_IRQHandlerでDMA無効化とエラー割り込みの無効化が行われているので復旧する必要がある */
	/* HAL_UART_Receive_DMAを呼ぶのが妥当 */

	hUartHandle->ErrorCode = HAL_UART_ERROR_NONE;
	hUartHandle->RxState = HAL_UART_STATE_BUSY_RX;

	/* Change DMA peripheral state */
	hUartHandle->hdmarx->State = HAL_DMA_STATE_BUSY;
	hUartHandle->hdmarx->ErrorCode = HAL_DMA_ERROR_NONE;

	/* Disable the peripheral */
	__HAL_DMA_DISABLE(hUartHandle->hdmarx);

	/* Configure DMA Channel data length */
	hUartHandle->hdmarx->Instance->CNDTR = UART_RX_BUF_SIZE;

	__HAL_DMA_DISABLE_IT(hUartHandle->hdmarx, DMA_IT_HT);
	__HAL_DMA_ENABLE_IT(hUartHandle->hdmarx, (DMA_IT_TC | DMA_IT_TE));

	  /* Enable the Peripheral */
	__HAL_DMA_ENABLE(hUartHandle->hdmarx);

	/* Enable the UART Parity Error interrupt */
	SET_BIT(hUartHandle->Instance->CR1, USART_CR1_PEIE);

	/* Enable the UART Error interrupt: (Frame error, noise error, overrun error) */
	SET_BIT(hUartHandle->Instance->CR3, USART_CR3_EIE);

	/* Enable the DMA transfer for the receiver request by setting the DMAR bit
	in the UART CR3 register */
	SET_BIT(hUartHandle->Instance->CR3, USART_CR3_DMAR);
#endif
}


/********************************************************************************/
/**
	@brief	UART_IsEmpty 関数

			UART受信バッファが空かどうかを返す

	@author	y.nakagawa
	@date	2017/11/13(月) 10:34:30

	@retval	Bool	TRUE:成 / FALSE:否
*/
/********************************************************************************/
PUBLIC
Bool
UART_IsEmpty( void )
{
	return (UART_rxRp + __HAL_DMA_GET_COUNTER(hUartHandle->hdmarx) == UART_RX_BUF_SIZE);
}


/********************************************************************************/
/**
	@brief	HAL_UART_ErrorCallback 関数

	@author	y.nakagawa
	@date	2017/03/21(火) 14:55:10

	@param[out]	huart	(UART_HandleTypeDef*)	※ 要コメント

	@return	なし (none)	※要コメント(voidの場合は省略可能)
*/
/********************************************************************************/
PUBLIC
void
HAL_UART_ErrorCallback(UART_HandleTypeDef *huart)
{
	UART_Error = huart->ErrorCode ;
#if defined(LOG_PRINTF)
	LOG_PRINTF( "[ERROR] UART error intterupt! (0x%08X)", UART_Error) ;
#endif
	switch ( UART_Error )
	{
	case HAL_UART_ERROR_PE	 :	   /**< Parity error		*/
	case HAL_UART_ERROR_NE	 :	   /**< Noise error 		*/
	case HAL_UART_ERROR_FE	 :	   /**< frame error 		*/
	case HAL_UART_ERROR_ORE  :	   /**< Overrun error		*/
		break ;
	case HAL_UART_ERROR_DMA  :	   /**< DMA transfer error	*/
		break ;
	default:
		break ;
	}
}


/********************************************************************************/
/**
	@brief	UART_Read 関数

	@author	y.nakagawa
	@date	2017/11/13(月) 10:34:19

	@return	int
*/
/********************************************************************************/
PUBLIC
int
UART_Read( void )
{
	int c = -1;		/* 受信データがない時は-1を返す */
	if ( UART_Error != 0 )
	{
		UART_Reset( ) ;
	}
	if ( !UART_IsEmpty() )
	{
		/* 受信データがある時 */
		c = UART_rxBuf[UART_rxRp++];
		if (UART_rxRp >= UART_RX_BUF_SIZE) {
			UART_rxRp = 0;
		}
	}
	return c;
}


/********************************************************************************/
/**
	@brief	UART_Write 関数
	@author	y.nakagawa
	@date	2016/10/13(木) 11:09:39
	@date	2016/10/13(木) 16:18:27 y.nakagawa 更新

	@param[in]	pcBuf	(uint8_t *)	バッファ
	@param[in]	ulLen	(uint16_t)	長さ

	@return	int	int
*/
/********************************************************************************/
PUBLIC
int
UART_Write(uint8_t *pcBuf, uint16_t ulLen)
{
	/* HALを使うと受信を取りこぼす？
	 * HAL_UART_Transmit( hUartHandle, (uint8_t *)pcBuf, ulLen, UART_TX_TIMEOUT ) ;
	 */
	uint32_t	loop = 0 ;
	char *	ptr = NULL ;

	ptr = (char *)pcBuf ;
	for ( loop = ulLen ; loop != 0 ; loop-- )
	{
		for ( ; (hUartHandle->Instance->ISR & UART_FLAG_TXE) != UART_FLAG_TXE ; ) ;
		hUartHandle->Instance->TDR = *ptr++ ;
	}
	/*
	 * これを確認するとHALと同じように受信を取りこぼす(HALとの差分)
	 * for ( ; (hUartHandle->Instance->ISR & UART_FLAG_TC) != UART_FLAG_TC ; ) ;
	 */
	return 0 ;
}


/********************************************************************************/
/**
	@brief	UART_Init 関数
	@author	y.nakagawa
	@date	2016/10/07(金) 17:34:17
	@date	2018/10/05(金) 17:25:31 y.nakagawa 更新

	@param[out]	handle	(UART_HandleTypeDef *)	※ 要コメント

	@return	なし (none)※要コメント(voidの場合は省略可能)
*/
/********************************************************************************/
PUBLIC
void
UART_Init( UART_HandleTypeDef * handle )
{
	hUartHandle = handle ;
	/* UART+DMA受信 */
	HAL_UART_Receive_DMA( hUartHandle, UART_rxBuf, UART_RX_BUF_SIZE);
}


/********************************************************************************/
/**
	@brief	UART_Terminate 関数

	@author	y.nakagawa
	@date	2016/10/07(金) 17:34:05

	@return	なし (none)	※要コメント(voidの場合は省略可能)
*/
/********************************************************************************/
PUBLIC
void
UART_Terminate(void)
{
	;
}


/********************************************************************************/
/**
	@brief	UART_SetBaudRate 関数

	@author	y.nakagawa
	@date	2016/10/07(金) 16:31:11

	@param[in]	baudRate	(uint32_t)	※ 要コメント

	@return	なし (none)	※要コメント(voidの場合は省略可能)
*/
/********************************************************************************/
PUBLIC
void
UART_SetBaudRate(uint32_t	baudRate)
{
	switch ( baudRate )
	{
	case 9600 :
	case 19200 :
	case 38400 :
	case 57600 :
	case 115200 :
		break ;
	default :
		baudRate = 115200 ;
		break ;
	}
	hUartHandle->Init.BaudRate = baudRate;
	hUartHandle->Init.WordLength = UART_WORDLENGTH_8B;
	hUartHandle->Init.StopBits = UART_STOPBITS_1;
	hUartHandle->Init.Parity = UART_PARITY_NONE;
	hUartHandle->Init.Mode = UART_MODE_TX_RX;
	hUartHandle->Init.HwFlowCtl = UART_HWCONTROL_NONE;
	hUartHandle->Init.OverSampling = UART_OVERSAMPLING_16;
	hUartHandle->Init.OneBitSampling = UART_ONE_BIT_SAMPLE_DISABLE;
	hUartHandle->AdvancedInit.AdvFeatureInit = UART_ADVFEATURE_NO_INIT;
	HAL_UART_Init(hUartHandle);
}


/********************************************************************************/
/**
	@brief	UART_EmergencyPrint 関数

	@author	y.nakagawa
	@date	2018/07/09(月) 11:14:25

	@param[out]	buf		(char *)	バッファ

	@return	なし (none)
*/
/********************************************************************************/
PUBLIC
inline
void
UART_EmergencyPrint( char * buf )
{
	while (*buf) {
		if (hUartHandle->Instance->ISR & UART_FLAG_TXE) {	/* 送信データレジスタが空だったら */
			hUartHandle->Instance->TDR = *buf++;
		}
	}
}

