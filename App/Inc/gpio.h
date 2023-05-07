/************************************************************************/
/* MODULE:	   GPIO モジュール											*/
/* COPYRIGHT (C) 										*/
/************************************************************************/
/**
	@file		gpio.h
	@brief		GPIO モジュール
	@author 	y.nakagawa

	$Id:$
*/

#ifndef    __GPIO_LIB_H__
#define    __GPIO_LIB_H__

/************************************************************************/
/* INCLUDE FILE SECTION 												*/
/************************************************************************/
/* nothing */

/************************************************************************/
/* MACRO DEFINITION SECTION 											*/
/************************************************************************/
#define	GPIO_CONFIG_HOLD				(0xFFFFFFFF)

/* LED3 (PB3) */
#define	GPIO_LED_PORT					GPIOB
#define	GPIO_LED_PIN					GPIO_PIN_3


/* Arduino A5 (PA6) */
#define	GPIO_PIN01_PORT					GPIOA
#define	GPIO_PIN01_PIN					GPIO_PIN_6



/************************************************************************/
/* TYPEDEF DEFINITION SECTION											*/
/************************************************************************/
typedef enum {
	PIN01,
	PINNUM,
} enum_gpio_id_t ;

typedef enum {
	GPIO_LOW,
	GPIO_HIGH
} GPIO_state_t;

typedef enum {
	GPIO_INPUT,
	GPIO_OUTPUT
} GPIO_mode_t;


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
void
GPIO_Init( void ) ;

extern
void
GPIO_PinConfigSet( enum_gpio_id_t id, uint32_t mode, uint32_t pull, uint32_t speed ) ;

extern
uint32_t
GPIO_PinConfigModeGet( enum_gpio_id_t id ) ;

extern
uint32_t
GPIO_PinConfigPullGet( enum_gpio_id_t id ) ;

extern
uint32_t
GPIO_PinConfigSpeedGet( enum_gpio_id_t id ) ;

extern
void
GPIO_PinWrite( enum_gpio_id_t id, GPIO_state_t state ) ;

extern
GPIO_state_t
GPIO_PinRead( enum_gpio_id_t id ) ;

extern
void
GPIO_LedWrite( GPIO_state_t state ) ;

#endif /*__GPIO_LIB_H__*/
