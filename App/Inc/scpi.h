/************************************************************************/
/* MODULE:     SCPI Lib                                                 */
/************************************************************************/
/**
	@file       SCPI.h
	@brief      SCPI Lib
	@author     Yasuhisa Nakagawa

	$Id:$
*/

#ifndef    __SCPI_H__
#define    __SCPI_H__

/************************************************************************/
/* INCLUDE FILE SECTION                                                 */
/************************************************************************/
#include "scpi.h"


/************************************************************************/
/* MACRO DEFINITION SECTION                                             */
/************************************************************************/
#define SCPI_INPUT_BUFFER_LENGTH 256
#define SCPI_ERROR_QUEUE_SIZE 17
#define SCPI_IDN1 "MANUFACTURE"
#define SCPI_IDN2 "INSTR2013"
#define SCPI_IDN3 NULL
#define SCPI_IDN4 "01-02"


/************************************************************************/
/* TYPEDEF DEFINITION SECTION                                           */
/************************************************************************/
/* nothing */

/************************************************************************/
/* GLOBAL DATA DEFINITION SECTION(extern)                               */
/************************************************************************/
extern const scpi_command_t scpi_commands[];
extern scpi_interface_t scpi_interface;
extern char scpi_input_buffer[];
extern scpi_error_t scpi_error_queue_data[];
extern scpi_t scpi_context;


/************************************************************************/
/* FUNCTION PROTOTYPE DEFINITION SECTION(PUBLIC)                        */
/************************************************************************/
/* nothing */

/************************************************************************/
/* FUNCTION PROTOTYPE DEFINITION SECTION(static)                       */
/************************************************************************/
/* nothing */

/************************************************************************/
/* FUNCTION PROTOTYPE DEFINITION SECTION(extern)                        */
/************************************************************************/
extern
size_t
SCPI_Write(scpi_t * context, const char * data, size_t len) ;

extern
int
SCPI_Error(scpi_t * context, int_fast16_t err);

extern
scpi_result_t
SCPI_Control(scpi_t * context, scpi_ctrl_name_t ctrl, scpi_reg_val_t val);

extern
scpi_result_t
SCPI_Reset(scpi_t * context);

extern
scpi_result_t
SCPI_Flush(scpi_t * context);




#endif /*__SCPI_H__*/
