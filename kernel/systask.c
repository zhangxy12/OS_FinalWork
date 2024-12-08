/*************************************************************************//**
 *****************************************************************************
 * @file   systask.c
 * @brief  
 * @author Forrest Y. Yu
 * @date   2007
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
#include "keyboard.h"
#include "proto.h"

PRIVATE int read_register(char reg_addr);
PRIVATE u32 get_rtc_time(struct time *t);



/*****************************************************************************
 *                                task_sys
 *****************************************************************************/
/**
 * <Ring 1> The main loop of TASK SYS.
 * 
 *****************************************************************************/
PUBLIC void task_sys()
{
	MESSAGE msg;
	struct time t;

	while (1) {
		send_recv(RECEIVE, ANY, &msg);
		int src = msg.source;

		switch (msg.type) {
		case GET_TICKS:
			msg.RETVAL = ticks;
			send_recv(SEND, src, &msg);
			break;
		case GET_PID:
			msg.type = SYSCALL_RET;
			msg.PID = src;
			send_recv(SEND, src, &msg);
			break;
		case GET_RTC_TIME:
			msg.type = SYSCALL_RET;
			get_rtc_time(&t);
			phys_copy(va2la(src, msg.BUF),
				  va2la(TASK_SYS, &t),
				  sizeof(t));
			send_recv(SEND, src, &msg);
			break;
		case GET_PROC_INFO:
			msg.type = SYSCALL_RET;
			phys_copy(va2la(src, msg.BUF),
				  va2la(TASK_SYS, &proc_table[msg.PID]),
				  sizeof(struct proc));
			send_recv(SEND, src, &msg);
            break;
		case KILL_PROC:
			{
				// 分配一个固定内存来存储目标进程名称
				msg.type = SYSCALL_RET;
				char target_name[20];  // 假设最大进程名称长度为20
				phys_copy(va2la(TASK_SYS, target_name), va2la(src, msg.BUF), 20);

				struct proc* target_proc = NULL;

				// 遍历进程表，查找进程名字匹配的进程
				for (int i = 0; i < NR_TASKS + NR_PROCS; i++) {
					if (proc_table[i].p_flags != FREE_SLOT && strcmp(proc_table[i].name, target_name) == 0) {
						target_proc = &proc_table[i];
						break;
					}
				}

				if (target_proc != NULL) {
				// 清理目标进程占用的文件描述符
				for (int i = 0; i < NR_FILES; i++) {
					if (target_proc->filp[i] != NULL) {
						close(i);  // 关闭文件描述符
						target_proc->filp[i] = NULL;  // 清空文件描述符指针
					}
				}

				// 释放目标进程占用的内存
				int target_pid = target_proc - proc_table;  // 获取目标进程的 PID
				if (target_pid >= NR_TASKS + NR_NATIVE_PROCS) {  // 判断是否是用户进程
					free_mem(target_pid);  // 释放目标进程的内存
				}

				target_proc->p_flags = HANGING;  // 将进程状态改为 HANGING

				// 返回成功
				msg.RETVAL = 0;  // 成功
			} else {
				// 找不到进程
				msg.RETVAL = -1;  // 失败，进程不存在
			}
				send_recv(SEND, src, &msg);
			}
			break;
		default:
			panic("unknown msg type");
			break;
		}
	}
}

/*****************************************************************************
 *                                get_rtc_time
 *****************************************************************************/
/**
 * Get RTC time from the CMOS
 * 
 * @return Zero.
 *****************************************************************************/
PRIVATE u32 get_rtc_time(struct time *t)
{
	t->year = read_register(YEAR);
	t->month = read_register(MONTH);
	t->day = read_register(DAY);
	t->hour = read_register(HOUR);
	t->minute = read_register(MINUTE);
	t->second = read_register(SECOND);

	if ((read_register(CLK_STATUS) & 0x04) == 0) {
		/* Convert BCD to binary (default RTC mode) */
		t->year = BCD_TO_DEC(t->year);
		t->month = BCD_TO_DEC(t->month);
		t->day = BCD_TO_DEC(t->day);
		t->hour = BCD_TO_DEC(t->hour);
		t->minute = BCD_TO_DEC(t->minute);
		t->second = BCD_TO_DEC(t->second);
	}

	t->year += 2000;

	return 0;
}

/*****************************************************************************
 *                                read_register
 *****************************************************************************/
/**
 * Read register from CMOS.
 * 
 * @param reg_addr 
 * 
 * @return 
 *****************************************************************************/
PRIVATE int read_register(char reg_addr)
{
	out_byte(CLK_ELE, reg_addr);
	return in_byte(CLK_IO);
}

