/**
 * @file system.h
 * @author y.nakagawa
 * @brief 
 * @version 0.1
 * @date 2023-05-05
 * 
 * @copyright Copyright (c) 2023
 * 
 */
#include <stdint.h>
#include <stdbool.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>

#include "gpio.h"
#include "usart.h"
#include "tim.h"

#define	PUBLIC
#define	Bool	bool
#define	TRUE	true
#define	FALSE	false

#define	MODEL_COMMAND_CRLF	"\r\n"
#define	ECHO_BACK

#define	HALT()	while(1);
extern void UART_AssertionFailed(const char *func, int line);
#define ASSERT(x)										\
	do {												\
		if ((x) == false) {									\
			UART_AssertionFailed(__func__, __LINE__);	\
		}												\
	} while (0)
 
