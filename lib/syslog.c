/*************************************************************************//**
 *****************************************************************************
 * @file   syslog.c
 * @brief  
 * @author Forrest Y. Yu
 * @date   Thu Nov 20 17:02:42 2008
 *****************************************************************************
 *****************************************************************************/

#include "type.h"
#include "stdio.h"
#include "const.h"
#include "protect.h"
#include "string.h"
#include "fs.h"
#include "proc.h"
#include "tty.h"
#include "console.h"
#include "global.h"
#include "proto.h"


/*****************************************************************************
 *                                syslog
 *****************************************************************************/
/**
 * Write log directly to the disk by sending message to FS.
 * 
 * @param fmt The format string.
 * 
 * @return How many chars have been printed.
 *****************************************************************************/
PUBLIC int syslog(const char *fmt, ...)
{
	int len_buf;
	char buf[STR_DEFAULT_LEN];

	va_list arg = (va_list)((char*)(&fmt) + 4); /**
						     * 4: size of `fmt' in
						     *    the stack
						     */
	len_buf = vsprintf(buf, fmt, arg);
	assert(strlen(buf) == len_buf);

	char log_type[10];
	for(int gg=0; gg<9; ++gg) {
		log_type[gg] = buf[gg];
	}
	log_type[9] = 0;
	printl("LOG_FLAGS: ");
	for(int gg=0; gg<4; ++gg) {
		printl("%d", LOG_FLAGS[gg]);
	}
	printl("\n");
	if(strcmp(log_type, "{tasktty}")==0) {
		if(!LOG_FLAGS[0]) {
			return -1;
		}
	} else if (strcmp(log_type, "{task_fs}")==0) {
		if(!LOG_FLAGS[1]) {
			return -1;
		}
	} else if (strcmp(log_type, "{task_mm}")==0) {
		if(!LOG_FLAGS[2]) {
			return -1;
		}
	} else if (strcmp(log_type, "{tasklog}")==0) {
		if(!LOG_FLAGS[3]) {
			return -1;
		}
	}

	struct time t;
	MESSAGE msg;
	msg.type = GET_RTC_TIME;
	msg.BUF= &t;
	send_recv(BOTH, TASK_SYS, &msg);
    // Read RTC registers
	// disable_int();
    // out_byte(CLK_ELE, YEAR);
    // t.year = in_byte(CLK_IO);
    // out_byte(CLK_ELE, MONTH);
    // t.month = in_byte(CLK_IO);
    // out_byte(CLK_ELE, DAY);
    // t.day = in_byte(CLK_IO);
    // out_byte(CLK_ELE, HOUR);
    // t.hour = in_byte(CLK_IO);
    // out_byte(CLK_ELE, MINUTE);
    // t.minute = in_byte(CLK_IO);
    // out_byte(CLK_ELE, SECOND);
    // t.second = in_byte(CLK_IO);
	// out_byte(CLK_ELE, CLK_STATUS);
    // if ((in_byte(CLK_IO) & 0x04) == 0) {
    //     t.year = BCD_TO_DEC(t.year);
    //     t.month = BCD_TO_DEC(t.month);
    //     t.day = BCD_TO_DEC(t.day);
    //     t.hour = BCD_TO_DEC(t.hour);
    //     t.minute = BCD_TO_DEC(t.minute);
    //     t.second = BCD_TO_DEC(t.second);
    // }
    // t.year += 2000;
	// enable_int();

	char time_buf[STR_DEFAULT_LEN];
	int len_time_buf = sprintf((char*)time_buf, "<%d-%02d-%02d %02d:%02d:%02d> ",
		t.year,
		t.month,
		t.day,
		t.hour,
		t.minute,
		t.second
	);

	char *ptr = time_buf + len_time_buf;
    for (int i = 0; i < len_buf; i++) {
        *ptr++ = buf[i];
    }
    *ptr = '\0';

	return filelog(time_buf);
	//return disklog(buf);
}
