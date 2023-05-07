/************************************************************************/
/* MODULE:		GPIO モジュール											*/
/* COPYRIGHT (C) 										*/
/************************************************************************/
/**
	@file		GPIO_Lib.c
	@brief		GPIO モジュール
	@author		y.nakagawa

	$Id:$

	@note	ポート名とピン名は、マクロの仕様上、
			GPIO_FOO_BAR_PORTとGPIO_FOO_BAR_PINという名前で定義されている必要がある。
*/

/************************************************************************/
/* INCLUDE FILE SECTION													*/
/************************************************************************/
#include "system.h"
#include "../../App/Inc/gpio.h"

/************************************************************************/
/* MACRO DEFINITION SECTION												*/
/************************************************************************/
/*
 * マクロはすべて、nameにFOO_BARと指定すると、
 * ポート名をGPIO_FOO_BAR_PORT、ピン名をGPIO_FOO_BAR_PINという名前に展開する。
 */

/* ポートの入出力設定用マクロ */
#define GPIO_SET_MODE(name, mode, pull, speed)	GPIO_SetMode(GPIO_##name##_PORT, GPIO_##name##_PIN, (mode), (pull), (speed))

/* ポートの出力値を設定するマクロ */
#define GPIO_WRITE(name, state)									\
	HAL_GPIO_WritePin(GPIO_##name##_PORT, GPIO_##name##_PIN, (state) == GPIO_LOW ? GPIO_PIN_RESET : GPIO_PIN_SET)

/* ポートの入力値を返すマクロ */
#define GPIO_READ(name)											\
	(HAL_GPIO_ReadPin(GPIO_##name##_PORT, GPIO_##name##_PIN) == GPIO_PIN_RESET ? GPIO_LOW : GPIO_HIGH)


/************************************************************************/
/* TYPEDEF DEFINITION SECTION											*/
/************************************************************************/
/* nothing */

/************************************************************************/
/* GLOBAL DATA DEFINITION SECTION(PUBLIC)								*/
/************************************************************************/
/* nothing */

/************************************************************************/
/* GLOBAL DATA DEFINITION SECTION(static)								*/
/************************************************************************/

/*
 * 初期値化および揮発性記憶領域
 */
static
GPIO_InitTypeDef
GPIO_InitStruct[ PINNUM ] = {
	{ .Pin = GPIO_PIN01_PIN, .Mode = GPIO_MODE_INPUT, .Pull = GPIO_NOPULL, .Speed = GPIO_SPEED_FREQ_LOW },
} ;


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
/* nothing */


/********************************************************************************/
/**
	@brief	GPIO_Init 関数

	@author	y.nakagawa
	@date	2018/10/15(月) 10:41:50

	@return	なし (none)
*/
/********************************************************************************/
PUBLIC
void
GPIO_Init( void )
{
	enum_gpio_id_t id ;
	GPIO_TypeDef *	port = NULL ;

	/* GPIO Ports Clock Enable */
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();

	/* Configure GPIO pin Output Level */
	for ( id = PIN01 ; id < PINNUM ; id++ )
	{
		GPIO_PinWrite( id, GPIO_LOW ) ;
	}

	/* Configure GPIO pins */
	for ( id = PIN01 ; id < PINNUM ; id++ )
	{
		switch ( id )
		{
		case PIN01 : port = GPIO_PIN01_PORT ; break ;
		default :
			break ;
		}
		HAL_GPIO_Init( port, &GPIO_InitStruct[ id ] );
	}
}


/********************************************************************************/
/**
	@brief	GPIO_PinConfigSet 関数

	@author	y.nakagawa
	@date	2018/10/12(金) 15:06:44

	@param[in]	id		(enum_gpio_id_t)	IOピン識別
	@param[in]	mode	(uint32_t)			モード
	@param[in]	pull	(uint32_t)			No Pull / Pull down / Pull up
	@param[in]	speed	(uint32_t)			IO速度

	@return	なし (none)

	@note	GPIO_CONFIG_HOLD(0xFFFFFFFF)を指定した設定はRAMの内容を反映する。
*/
/********************************************************************************/
PUBLIC
void
GPIO_PinConfigSet( enum_gpio_id_t id, uint32_t mode, uint32_t pull, uint32_t speed )
{
	GPIO_TypeDef *	port = NULL ;

	ASSERT( id < PINNUM ) ;
	switch ( id )
	{
	case PIN01 : port = GPIO_PIN01_PORT ; break ;
	default :
		break ;
	}
	if ( port != NULL )
	{
		if ( GPIO_CONFIG_HOLD != mode )		GPIO_InitStruct[ id ].Mode = mode  ;
		if ( GPIO_CONFIG_HOLD != pull )		GPIO_InitStruct[ id ].Pull = pull ;
		if ( GPIO_CONFIG_HOLD != speed )	GPIO_InitStruct[ id ].Speed = speed ;
		HAL_GPIO_Init( port, &GPIO_InitStruct[ id ] );
	}
}


/********************************************************************************/
/**
	@brief	GPIO_PinConfigModeGet 関数

	@author	y.nakagawa
	@date	2018/10/15(月) 11:19:26

	@param[in]	id		(enum_gpio_id_t)	IOピン識別

	@return	uint32_t
*/
/********************************************************************************/
PUBLIC
uint32_t
GPIO_PinConfigModeGet( enum_gpio_id_t id )
{
	return GPIO_InitStruct[ id ].Mode ;
}


/********************************************************************************/
/**
	@brief	GPIO_PinConfigPullGet 関数

	@author	y.nakagawa
	@date	2018/10/15(月) 11:19:31

	@param[in]	id		(enum_gpio_id_t)	IOピン識別

	@return	uint32_t
*/
/********************************************************************************/
PUBLIC
uint32_t
GPIO_PinConfigPullGet( enum_gpio_id_t id )
{
	return GPIO_InitStruct[ id ].Pull ;
}


/********************************************************************************/
/**
	@brief	GPIO_PinConfigSpeedGet 関数

	@author	y.nakagawa
	@date	2018/10/15(月) 11:19:34

	@param[in]	id		(enum_gpio_id_t)	IOピン識別

	@return	uint32_t
*/
/********************************************************************************/
PUBLIC
uint32_t
GPIO_PinConfigSpeedGet( enum_gpio_id_t id )
{
	return GPIO_InitStruct[ id ].Speed ;
}


/********************************************************************************/
/**
	@brief	GPIO_PinWrite 関数

	@author	y.nakagawa
	@date	2018/10/12(金) 15:06:51

	@param[in]	id		(enum_gpio_id_t)	IOピン識別
	@param[in]	state	(GPIO_state_t)		出力レベル

	@return	なし (none)
*/
/********************************************************************************/
PUBLIC
void
GPIO_PinWrite( enum_gpio_id_t id, GPIO_state_t state )
{
	ASSERT( id < PINNUM ) ;
	switch ( id )
	{
	case PIN01 : GPIO_WRITE( PIN01, state ) ; break ;
	default :
		break ;
	}
}


/********************************************************************************/
/**
	@brief	GPIO_PinRead 関数

	@author	y.nakagawa
	@date	2018/10/12(金) 15:06:55

	@param[in]	id		(enum_gpio_id_t)	IOピン識別

	@return	GPIO_state_t
*/
/********************************************************************************/
PUBLIC
GPIO_state_t
GPIO_PinRead( enum_gpio_id_t id )
{
	GPIO_state_t state = GPIO_LOW;

	ASSERT( id < PINNUM ) ;
	switch ( id )
	{
	case PIN01 : state = GPIO_READ( PIN01 ) ; break ;
	default :
		break ;
	}
	return state ;
}


/********************************************************************************/
/**
	@brief	GPIO_LedWrite 関数

	@author	y.nakagawa
	@date	2018/10/12(金) 15:07:00

	@param[in]	state	(GPIO_state_t)	出力レベル

	@return	なし (none)
*/
/********************************************************************************/
PUBLIC
void
GPIO_LedWrite( GPIO_state_t state )
{
	GPIO_WRITE( LED, state ) ;
}


