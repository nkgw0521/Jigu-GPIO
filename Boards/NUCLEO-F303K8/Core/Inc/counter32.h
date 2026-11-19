#ifndef F303_COUNTER32_H
#define F303_COUNTER32_H
#include "tim.h"
HAL_StatusTypeDef counter32_start(void);
void counter32_stop(void);
void counter32_set(uint32_t value);
uint32_t counter32_get(void);
void counter32_on_overflow(void);
#endif
