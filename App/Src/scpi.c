/**
 *	@file	   SCPI.c
 *	@brief	  SCPI Lib
 *	@author	 Yasuhisa Nakagawa
 *
 *	$Id:$
 */

/************************************************************************/
/* INCLUDE FILE SECTION													*/
/************************************************************************/
#include "system.h"
#include "scpi/scpi.h"
#include "scpi.h"
#include "../../App/Inc/gpio.h"
#include "uart_lib.h"

/************************************************************************/
/* MACRO DEFINITION SECTION												*/
/************************************************************************/
#ifdef DEBUG
#define LOG_SCPI_PRINTF(...) \
	do { printf(__VA_ARGS__); } while(0)
#else
#define LOG_SCPI_PRINTF(...) \
	do {} while(0)
#endif

/************************************************************************/
/* TYPEDEF DEFINITION SECTION											*/
/************************************************************************/
struct _scpi_pwm_param_t {
	uint32_t	freq;
	uint32_t	width;
	Bool		polarity;
	Bool		start;
	uint32_t	count;
	uint32_t	numbers;	/* PWM:Numbers 用 */
};
typedef struct _scpi_pwm_param_t	scpi_pwm_param_t;

/************************************************************************/
/* GLOBAL DATA DEFINITION SECTION(PUBLIC)								*/
/************************************************************************/
scpi_interface_t scpi_interface = {
	.error = SCPI_Error,
	.write = SCPI_Write,
	.control = SCPI_Control,
	.flush = SCPI_Flush,
	.reset = SCPI_Reset,
};

char scpi_input_buffer[SCPI_INPUT_BUFFER_LENGTH];
scpi_error_t scpi_error_queue_data[SCPI_ERROR_QUEUE_SIZE];

scpi_t scpi_context;

/************************************************************************/
/* GLOBAL DATA DEFINITION SECTION(static)							  */
/************************************************************************/
static
enum_gpio_id_t
gGPIO_PinId = PIN01 ;

static
scpi_pwm_param_t pwm_param = { 50, 1, 1, 0, 0, 0 } ;


const scpi_choice_def_t	tblGpioMode[] = {
	{	"IN",			GPIO_MODE_INPUT	},
	{	"OUTPushpull",	GPIO_MODE_OUTPUT_PP	},
	{	"OUTOpendrain",	GPIO_MODE_OUTPUT_OD	},
	SCPI_CHOICE_LIST_END
} ;

const scpi_choice_def_t	tblGpioPull[] = {
	{	"No",			GPIO_NOPULL	},
	{	"Down",			GPIO_PULLDOWN	},
	{	"Up",			GPIO_PULLUP	},
	SCPI_CHOICE_LIST_END
};

const scpi_choice_def_t	tblGpioSpeed[] = {
	{	"Low",			GPIO_SPEED_FREQ_LOW	},
	{	"Medium",		GPIO_SPEED_FREQ_MEDIUM	},
	{	"Hight",		GPIO_SPEED_FREQ_HIGH	},
	{	"Veryhight",	GPIO_SPEED_FREQ_VERY_HIGH	},
	SCPI_CHOICE_LIST_END
} ;

const scpi_choice_def_t tblPwmPolarity[] = {
	{	"Lo",			0	},
	{	"Hi",			1	},
	SCPI_CHOICE_LIST_END
};


/************************************************************************/
/* GLOBAL DATA DEFINITION SECTION(extern)							   */
/************************************************************************/
/* nothing */

/************************************************************************/
/* FUNCTION PROTOTYPE DEFINITION SECTION(PUBLIC)						*/
/************************************************************************/
extern void pwm_numbers_set(uint32_t numbers);
extern void pwm_shot_count_reset(void);
extern void pwm_run_state_set(bool running);

/************************************************************************/
/* FUNCTION PROTOTYPE DEFINITION SECTION(static)					   */
/************************************************************************/
static
scpi_result_t
My_CoreTstQ(scpi_t * context) ;

static
scpi_result_t
SCPI_SystemLed( scpi_t * context ) ;

static
scpi_result_t
SCPI_GpioSelect( scpi_t * context ) ;
static
scpi_result_t
SCPI_GpioSelectQ( scpi_t * context ) ;

static
scpi_result_t
SCPI_GpioDirection( scpi_t * context ) ;
static
scpi_result_t
SCPI_GpioDirectionQ( scpi_t * context ) ;

static
scpi_result_t
SCPI_GpioLevel( scpi_t * context ) ;
static
scpi_result_t
SCPI_GpioLevelQ( scpi_t * context ) ;

static
scpi_result_t
SCPI_GpioPull( scpi_t * context ) ;
static
scpi_result_t
SCPI_GpioPullQ( scpi_t * context ) ;

static
scpi_result_t
SCPI_GpioSpeed( scpi_t * context ) ;
static
scpi_result_t
SCPI_GpioSpeedQ( scpi_t * context ) ;

static
scpi_result_t
SCPI_PwmFrequency( scpi_t * contest ) ;
static
scpi_result_t
SCPI_PwmFrequencyQ( scpi_t * context ) ;

static
scpi_result_t
SCPI_PwmWidth( scpi_t * context ) ;
static
scpi_result_t
SCPI_PwmWidthQ( scpi_t * context ) ;

static
scpi_result_t
SCPI_PwmPolarity( scpi_t * context ) ;
static
scpi_result_t
SCPI_PwmPolarityQ( scpi_t * context ) ;

static
scpi_result_t
SCPI_PwmStart( scpi_t * context );
static
scpi_result_t
SCPI_PwmStartQ( scpi_t * context );
static
scpi_result_t
SCPI_PwmCounter( scpi_t * context );
static
scpi_result_t
SCPI_PwmCounterQ( scpi_t * context );

static
scpi_result_t
SCPI_PwmNumbers( scpi_t * context );
static
scpi_result_t
SCPI_PwmNumbersQ( scpi_t * context );

/************************************************************************/
/* FUNCTION PROTOTYPE DEFINITION SECTION(extern)						*/
/************************************************************************/
/* nothing */

const scpi_command_t scpi_commands[] = {
	/* IEEE Mandated Commands (SCPI std V1999.0 4.1.1) */
	{ .pattern = "*CLS", .callback = SCPI_CoreCls,},
	{ .pattern = "*ESE", .callback = SCPI_CoreEse,},
	{ .pattern = "*ESE?", .callback = SCPI_CoreEseQ,},
	{ .pattern = "*ESR?", .callback = SCPI_CoreEsrQ,},
	{ .pattern = "*IDN?", .callback = SCPI_CoreIdnQ,},
	{ .pattern = "*OPC", .callback = SCPI_CoreOpc,},
	{ .pattern = "*OPC?", .callback = SCPI_CoreOpcQ,},
	{ .pattern = "*RST", .callback = SCPI_CoreRst,},
	{ .pattern = "*SRE", .callback = SCPI_CoreSre,},
	{ .pattern = "*SRE?", .callback = SCPI_CoreSreQ,},
	{ .pattern = "*STB?", .callback = SCPI_CoreStbQ,},
	{ .pattern = "*TST?", .callback = My_CoreTstQ,},
	{ .pattern = "*WAI", .callback = SCPI_CoreWai,},

	/* Required SCPI commands (SCPI std V1999.0 4.2.1) */
	{.pattern = "SYSTem:ERRor[:NEXT]?", .callback = SCPI_SystemErrorNextQ,},
	{.pattern = "SYSTem:ERRor:COUNt?", .callback = SCPI_SystemErrorCountQ,},
	{.pattern = "SYSTem:VERSion?", .callback = SCPI_SystemVersionQ,},

	/* Extended command */
	{ .pattern = "SYSTem:LED"/* {0|1|ON|OFF} */, .callback = SCPI_SystemLed, },
	/* GPIO command */
	{ .pattern = "GPIO:SELect"/* {1|2|3|4|5|6|7|8} */, .callback = SCPI_GpioSelect, },
	{. pattern = "GPIO:SELect?", .callback = SCPI_GpioSelectQ, },

	{ .pattern = "GPIO:DIRection"/* {IN|OUTPushpull|OUTOpendrain} */, .callback = SCPI_GpioDirection, },
	{ .pattern = "GPIO:DIRection?", .callback = SCPI_GpioDirectionQ, },

	{ .pattern = "GPIO:LEVel"/* {0|1|Lo|Hi} */, .callback = SCPI_GpioLevel, },
	{ .pattern = "GPIO:LEVel?", .callback = SCPI_GpioLevelQ, },

	{ .pattern = "GPIO:PULl"/* {No|Down|Up} */, .callback = SCPI_GpioPull, },
	{ .pattern = "GPIO:PULl?", .callback = SCPI_GpioPullQ, },

	{ .pattern = "GPIO:Speed"/* {Low|Medium|High|Veryhigh} */, .callback = SCPI_GpioSpeed, },
	{ .pattern = "GPIO:Speed?", .callback = SCPI_GpioSpeedQ, },

	/** PWM Command */
	{ .pattern = "PWM:Frequency", .callback = SCPI_PwmFrequency, },
	{ .pattern = "PWM:Frequency?", .callback = SCPI_PwmFrequencyQ, },
	{ .pattern = "PWM:Width", .callback = SCPI_PwmWidth, },
	{ .pattern = "PWM:Width?", .callback = SCPI_PwmWidthQ, },
	{ .pattern = "PWM:POLarity"/* {0|1} */, .callback = SCPI_PwmPolarity, },
	{ .pattern = "PWM:POLarity?", .callback = SCPI_PwmPolarityQ, },
	{ .pattern = "PWM:STart", .callback = SCPI_PwmStart, },
	{ .pattern = "PWM:STart?", .callback = SCPI_PwmStartQ, },
	{ .pattern = "PWM:Counter", .callback = SCPI_PwmCounter },
	{ .pattern = "PWM:Counter?", .callback = SCPI_PwmCounterQ, },
	{ .pattern = "PWM:Numbers",  .callback = SCPI_PwmNumbers,  },
	{ .pattern = "PWM:Numbers?", .callback = SCPI_PwmNumbersQ, },
	SCPI_CMD_LIST_END
};


void SetPulseCount(uint32_t cnt)
{
	__disable_irq();
	__HAL_TIM_SET_COUNTER( &htim2, cnt ) ;
	__enable_irq();
}

uint32_t GetPulseCount(void)
{
	uint32_t val;
	__disable_irq();
	val = __HAL_TIM_GET_COUNTER(&htim2);
	__enable_irq();

	return val;
}

void ResetPulseCount( void )
{
	__disable_irq();
	__HAL_TIM_SET_COUNTER( &htim2, 0 ) ;
	__enable_irq();
}

size_t
SCPI_Write(scpi_t * context, const char * data, size_t len)
{
	(void) context;
	UART_Put( (char *)data, len ) ;
	return 1 ;
}


scpi_result_t
SCPI_Flush(scpi_t * context)
{
	(void) context;

	return SCPI_RES_OK;
}


int
SCPI_Error(scpi_t * context, int_fast16_t err)
{
	(void) context;

	LOG_SCPI_PRINTF( "**ERROR: %d, \"%s\"\r\n", (int16_t) err, SCPI_ErrorTranslate(err) );
	return 0;
}


scpi_result_t
SCPI_Control(scpi_t * context, scpi_ctrl_name_t ctrl, scpi_reg_val_t val)
{
	(void) context;

	if (SCPI_CTRL_SRQ == ctrl) {
		LOG_SCPI_PRINTF( "**SRQ: 0x%X (%d)\r\n", val, val);
	} else {
		LOG_SCPI_PRINTF( "**CTRL %02x: 0x%X (%d)\r\n", ctrl, val, val);
	}
	return SCPI_RES_OK;
}


scpi_result_t
SCPI_Reset(scpi_t * context)
{
	(void) context;

	LOG_SCPI_PRINTF( "**Reset\r\n");
	return SCPI_RES_OK;
}


void
pwm_start_state_set(Bool start)
{
    pwm_param.start = start;
}

/**
 * Reimplement IEEE488.2 *TST?
 *
 * Result should be 0 if everything is ok
 * Result should be 1 if something goes wrong
 *
 * Return SCPI_RES_OK
 */
static
scpi_result_t
My_CoreTstQ(scpi_t * context)
{
	SCPI_ResultInt32(context, 0);

	return SCPI_RES_OK;
}

void
SCPI_PwmReset( void )
{
	HAL_TIM_Base_Stop_IT(&htim1);
	HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
	HAL_TIM_Base_Stop_IT(&htim2);	/* カウンタ停止（外部パルスでカウント） */
	pwm_setup( pwm_param.freq, pwm_param.width, pwm_param.polarity ) ;
}

static
scpi_result_t
SCPI_SystemLed( scpi_t * context )
{
	scpi_result_t result = SCPI_RES_OK ;
	scpi_bool_t param1 ;

	/* read first parameter if present */
	if ( !SCPI_ParamBool(context, &param1, TRUE) )
	{
		result =  SCPI_RES_ERR;
	}
	else
	{
		GPIO_LedWrite( ( param1 ) ? GPIO_HIGH : GPIO_LOW ) ;
	}
	return result ;
}


static
scpi_result_t
SCPI_GpioSelect( scpi_t * context )
{
	scpi_result_t result = SCPI_RES_OK ;
	scpi_number_t param1;

	if ( SCPI_ParamNumber(context, scpi_special_numbers_def, &param1, TRUE) )
	{
		if ( (0 < param1.content.value) && (param1.content.value < (PINNUM+1)) )
		{
			gGPIO_PinId = (enum_gpio_id_t)(param1.content.value - 1) ;
		}
		else
		{
			result =  SCPI_RES_ERR;
		}
	}
	else
	{
		result =  SCPI_RES_ERR;
	}
	return result ;
}


static
scpi_result_t
SCPI_GpioSelectQ( scpi_t * context )
{
	SCPI_ResultInt32( context, gGPIO_PinId + 1 ) ;
	return SCPI_RES_OK;
}


static
scpi_result_t
SCPI_GpioDirection( scpi_t * context )
{
	scpi_result_t result = SCPI_RES_OK ;
	int32_t param;

	if ( SCPI_ParamChoice(context, tblGpioMode, &param, TRUE) )
	{
		GPIO_PinConfigSet( gGPIO_PinId, param, GPIO_CONFIG_HOLD, GPIO_CONFIG_HOLD ) ;
	}
	else
	{
		result = SCPI_RES_ERR ;
	}
	return result ;
}


static
scpi_result_t
SCPI_GpioDirectionQ( scpi_t * context )
{
	const char * name;
	uint32_t mode = GPIO_PinConfigModeGet( gGPIO_PinId ) ;

	SCPI_ChoiceToName( tblGpioMode, mode, &name);
	SCPI_ResultCharacters( context, name, strlen(name) ) ;

	return SCPI_RES_OK ;
}


static
scpi_result_t
SCPI_GpioLevel( scpi_t * context )
{
	scpi_result_t result = SCPI_RES_OK ;
	scpi_bool_t param1 ;

	/* read first parameter if present */
	if ( !SCPI_ParamBool(context, &param1, TRUE) )
	{
		result =  SCPI_RES_ERR;
	}
	else
	{
		GPIO_PinWrite( gGPIO_PinId, ( param1 ) ? GPIO_HIGH : GPIO_LOW ) ;
	}
	return result ;
}


static
scpi_result_t
SCPI_GpioLevelQ( scpi_t * context )
{
	GPIO_state_t state = GPIO_PinRead( gGPIO_PinId ) ;

	SCPI_ResultInt32( context, state ) ;
	return SCPI_RES_OK;;
}


static
scpi_result_t
SCPI_GpioPull( scpi_t * context )
{
	scpi_result_t result = SCPI_RES_OK ;
	int32_t param;

	if ( SCPI_ParamChoice(context, tblGpioPull, &param, TRUE) )
	{
		GPIO_PinConfigSet( gGPIO_PinId, GPIO_CONFIG_HOLD, param, GPIO_CONFIG_HOLD ) ;
	}
	else
	{
		result = SCPI_RES_ERR ;
	}
	return result ;
}


static
scpi_result_t
SCPI_GpioPullQ( scpi_t * context )
{
	const char * name;
	uint32_t mode = GPIO_PinConfigPullGet( gGPIO_PinId ) ;

	SCPI_ChoiceToName( tblGpioPull, mode, &name);
	SCPI_ResultCharacters( context, name, strlen(name) ) ;

	return SCPI_RES_OK ;
}


static
scpi_result_t
SCPI_GpioSpeed( scpi_t * context )
{
	scpi_result_t result = SCPI_RES_OK ;
	int32_t param;

	if ( SCPI_ParamChoice(context, tblGpioSpeed, &param, TRUE) )
	{
		GPIO_PinConfigSet( gGPIO_PinId, GPIO_CONFIG_HOLD, GPIO_CONFIG_HOLD, param ) ;
	}
	else
	{
		result = SCPI_RES_ERR ;
	}
	return result ;
}


static
scpi_result_t
SCPI_GpioSpeedQ( scpi_t * context )
{
	const char * name;
	uint32_t mode = GPIO_PinConfigSpeedGet( gGPIO_PinId ) ;

	SCPI_ChoiceToName( tblGpioSpeed, mode, &name);
	SCPI_ResultCharacters( context, name, strlen(name) ) ;

	return SCPI_RES_OK ;
}

/**
static
scpi_result_t
SCPI_XxxYyy( scpi_t * context )
{
	scpi_result_t result = SCPI_RES_OK ;
	return result ;
}
*/

static
scpi_result_t
SCPI_PwmFrequency( scpi_t * context )
{
	scpi_result_t result = SCPI_RES_OK ;
	scpi_number_t param1;

	if ( SCPI_ParamNumber(context, scpi_special_numbers_def, &param1, TRUE) )
	{
		pwm_param.freq = param1.content.value ;
		SCPI_PwmReset();
	}
	else
	{
		result =  SCPI_RES_ERR;
	}
	return result ;
}
static
scpi_result_t
SCPI_PwmFrequencyQ( scpi_t * context )
{
	scpi_result_t result = SCPI_RES_OK ;
	SCPI_ResultInt32( context, pwm_param.freq ) ;
	return result ;
}


static
scpi_result_t
SCPI_PwmWidth( scpi_t * context )
{
	scpi_result_t result = SCPI_RES_OK ;
	scpi_number_t param1;

	if ( SCPI_ParamNumber(context, scpi_special_numbers_def, &param1, TRUE) )
	{
		pwm_param.width = param1.content.value ;
		SCPI_PwmReset();
	}
	else
	{
		result =  SCPI_RES_ERR;
	}
	return result ;
}
static
scpi_result_t
SCPI_PwmWidthQ( scpi_t * context )
{
	scpi_result_t result = SCPI_RES_OK ;
	SCPI_ResultInt32( context, pwm_param.width ) ;
	return result ;
}


static
scpi_result_t
SCPI_PwmPolarity( scpi_t * context )
{
	scpi_result_t result = SCPI_RES_OK ;
	scpi_bool_t	param1 ;
	if ( !SCPI_ParamBool(context, &param1, TRUE) )
	{
		result =  SCPI_RES_ERR;
	}
	else
	{
		pwm_param.polarity = param1;
		SCPI_PwmReset();
	}
	return result ;
}
static
scpi_result_t
SCPI_PwmPolarityQ( scpi_t * context )
{
	scpi_result_t result = SCPI_RES_OK ;
	SCPI_ResultBool(context,pwm_param.polarity);
	return result ;
}

static
scpi_result_t
SCPI_PwmStart( scpi_t * context )
{
	scpi_result_t result = SCPI_RES_OK;
	scpi_bool_t param1;

	if (!SCPI_ParamBool(context, &param1, TRUE))
	{
		result = SCPI_RES_ERR;
	}
	else
	{
		pwm_param.start = param1;

		if (pwm_param.start)
		{
			pwm_setup(pwm_param.freq, pwm_param.width, pwm_param.polarity);

			//pwm_numbers_set( (1 < pwm_param.numbers) ? pwm_param.numbers - 1 : pwm_param.numbers );
			pwm_numbers_set( pwm_param.numbers );
			pwm_shot_count_reset();
			pwm_run_state_set(true);

			__HAL_TIM_CLEAR_FLAG(&htim2, TIM_FLAG_UPDATE);
			HAL_TIM_Base_Start_IT(&htim2);	/* カウンタ開始（外部パルスでカウント） */

			__HAL_TIM_SET_COUNTER(&htim1, 0);
			__HAL_TIM_CLEAR_FLAG(&htim1, TIM_FLAG_UPDATE);
			if (HAL_TIM_PWM_Start(&htim1, TIM_CHANNEL_1) != HAL_OK) Error_Handler();
			if (HAL_TIM_Base_Start_IT(&htim1) != HAL_OK) Error_Handler();
		}
		else
		{
			HAL_TIM_Base_Stop_IT(&htim2);	/* カウンタ停止（外部パルスでカウント） */

			HAL_TIM_Base_Stop_IT(&htim1);
			HAL_TIM_PWM_Stop(&htim1, TIM_CHANNEL_1);
			pwm_run_state_set(false);
		}
	}

	return result;
}

static
scpi_result_t
SCPI_PwmStartQ( scpi_t * context )
{
	scpi_result_t result = SCPI_RES_OK ;
	SCPI_ResultBool(context,pwm_param.start);
	return result ;
}

static
scpi_result_t
SCPI_PwmCounter( scpi_t * context )
{
	scpi_result_t result = SCPI_RES_OK ;
	scpi_number_t param1;

	if ( SCPI_ParamNumber(context, scpi_special_numbers_def, &param1, TRUE) )
	{
		pwm_param.count = param1.content.value ;
		SetPulseCount( pwm_param.count & 0x0000FFFF ) ;
	}
	else
	{
		result =  SCPI_RES_ERR;
	}
	return result ;
}

static
scpi_result_t
SCPI_PwmCounterQ( scpi_t * context )
{
	pwm_param.count = GetPulseCount() ;
	scpi_result_t result = SCPI_RES_OK ;
	SCPI_ResultUInt32Base( context, pwm_param.count, 10 ) ;
	return result ;
}

static
scpi_result_t
SCPI_PwmNumbers( scpi_t * context )
{
	scpi_result_t result = SCPI_RES_OK;
	scpi_number_t param1;

	if (SCPI_ParamNumber(context, scpi_special_numbers_def, &param1, TRUE))
	{
		if (param1.content.value < 0) {
			result = SCPI_RES_ERR;
		} else {
			pwm_param.numbers = (uint32_t)param1.content.value ;
		}
	}
	else
	{
		result = SCPI_RES_ERR;
	}
	return result;
}

static
scpi_result_t
SCPI_PwmNumbersQ( scpi_t * context )
{
	SCPI_ResultUInt32Base(context, pwm_param.numbers, 10);
	return SCPI_RES_OK;
}
