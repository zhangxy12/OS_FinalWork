
/*++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                            main.c
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++
                                                    Forrest Yu, 2005
++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++*/

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

/*use chceck*/
#define STATIC_CHECK  1
typedef struct {
    char name[128];           // 文件名
    int check_value;   // 偶校验
} Check;

/*****************************************************************************
 *                               kernel_main
 *****************************************************************************/
/**
 * jmp from kernel.asm::_start. 
 * 
 *****************************************************************************/
PUBLIC int kernel_main()
{
	disp_str("\n~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~\n");

	int i, j, eflags, prio;
        u8  rpl;
        u8  priv; /* privilege */

	struct task * t;
	struct proc * p = proc_table;

	char * stk = task_stack + STACK_SIZE_TOTAL;

	for (i = 0; i < NR_TASKS + NR_PROCS; i++,p++,t++) {
		if (i >= NR_TASKS + NR_NATIVE_PROCS) {
			p->p_flags = FREE_SLOT;
			continue;
		}

	        if (i < NR_TASKS) {     /* TASK */
                        t	= task_table + i;
                        priv	= PRIVILEGE_TASK;
                        rpl     = RPL_TASK;
                        eflags  = 0x1202;/* IF=1, IOPL=1, bit 2 is always 1 */
			prio    = 15;
                }
                else {                  /* USER PROC */
                        t	= user_proc_table + (i - NR_TASKS);
                        priv	= PRIVILEGE_USER;
                        rpl     = RPL_USER;
                        eflags  = 0x202;	/* IF=1, bit 2 is always 1 */
			prio    = 5;
                }

		strcpy(p->name, t->name);	/* name of the process */
		p->p_parent = NO_TASK;

		if (strcmp(t->name, "INIT") != 0) {
			p->ldts[INDEX_LDT_C]  = gdt[SELECTOR_KERNEL_CS >> 3];
			p->ldts[INDEX_LDT_RW] = gdt[SELECTOR_KERNEL_DS >> 3];

			/* change the DPLs */
			p->ldts[INDEX_LDT_C].attr1  = DA_C   | priv << 5;
			p->ldts[INDEX_LDT_RW].attr1 = DA_DRW | priv << 5;
		}
		else {		/* INIT process */
			unsigned int k_base;
			unsigned int k_limit;
			int ret = get_kernel_map(&k_base, &k_limit);
			assert(ret == 0);
			init_desc(&p->ldts[INDEX_LDT_C],
				  0, /* bytes before the entry point
				      * are useless (wasted) for the
				      * INIT process, doesn't matter
				      */
				  (k_base + k_limit) >> LIMIT_4K_SHIFT,
				  DA_32 | DA_LIMIT_4K | DA_C | priv << 5);

			init_desc(&p->ldts[INDEX_LDT_RW],
				  0, /* bytes before the entry point
				      * are useless (wasted) for the
				      * INIT process, doesn't matter
				      */
				  (k_base + k_limit) >> LIMIT_4K_SHIFT,
				  DA_32 | DA_LIMIT_4K | DA_DRW | priv << 5);
		}

		p->regs.cs = INDEX_LDT_C << 3 |	SA_TIL | rpl;
		p->regs.ds =
			p->regs.es =
			p->regs.fs =
			p->regs.ss = INDEX_LDT_RW << 3 | SA_TIL | rpl;
		p->regs.gs = (SELECTOR_KERNEL_GS & SA_RPL_MASK) | rpl;
		p->regs.eip	= (u32)t->initial_eip;
		p->regs.esp	= (u32)stk;
		p->regs.eflags	= eflags;

		p->ticks = p->priority = prio;

		p->p_flags = 0;
		p->p_msg = 0;
		p->p_recvfrom = NO_TASK;
		p->p_sendto = NO_TASK;
		p->has_int_msg = 0;
		p->q_sending = 0;
		p->next_sending = 0;

		for (j = 0; j < NR_FILES; j++)
			p->filp[j] = 0;

		stk -= t->stacksize;
	}

	k_reenter = 0;
	ticks = 0;
	first_log = 0;
	log_fd = -1;
	LOG_FLAGS[0] = LOG_FLAGS[1] = LOG_FLAGS[2] = LOG_FLAGS[3] = 1;

	p_proc_ready	= proc_table;

	init_clock();
        init_keyboard();

	restart();

	while(1){}
}


/*****************************************************************************
 *                                get_ticks
 *****************************************************************************/
PUBLIC int get_ticks()
{
	MESSAGE msg;
	reset_msg(&msg);
	msg.type = GET_TICKS;
	send_recv(BOTH, TASK_SYS, &msg);
	return msg.RETVAL;
}


/**
 * @struct posix_tar_header
 * Borrowed from GNU `tar'
 */
struct posix_tar_header
{				/* byte offset */
	char name[100];		/*   0 */
	char mode[8];		/* 100 */
	char uid[8];		/* 108 */
	char gid[8];		/* 116 */
	char size[12];		/* 124 */
	char mtime[12];		/* 136 */
	char chksum[8];		/* 148 */
	char typeflag;		/* 156 */
	char linkname[100];	/* 157 */
	char magic[6];		/* 257 */
	char version[2];	/* 263 */
	char uname[32];		/* 265 */
	char gname[32];		/* 297 */
	char devmajor[8];	/* 329 */
	char devminor[8];	/* 337 */
	char prefix[155];	/* 345 */
	/* 500 */
};

// 计算偶校验，通过异或计算校验位
int calculate_even_parity(unsigned char byte) {
    int parity = 0;
    
    // 对每个字节进行按位异或计算所有位的校验
    for (int i = 0; i < 8; i++) {
        parity ^= (byte >> i) & 1; // 对每一位进行异或操作
    }

    // 返回异或的结果
    return parity;
}
/*****************************************************************************
 *                                untar
 *****************************************************************************/
/**
 * Extract the tar file and store them.
 * 
 * @param filename The tar file.
 *****************************************************************************/
void untar(const char * filename)
{
	printf("[extract `%s'\n", filename);
	int fd = open(filename, O_RDWR);
	
	int check_file_fd;
    if (STATIC_CHECK) {
        check_file_fd = open("check", O_RDWR);  //  check存储每个可执行文件的校验值
        if (check_file_fd == -1) {
            printf("creating check file\n");
            check_file_fd = open("check", O_CREAT | O_RDWR);
        }
    }

	char temp_filename[32] = {0};
	assert(fd != -1);
	char buf[SECTOR_SIZE * 16];
	int chunk = sizeof(buf);
	int i = 0;
	int bytes = 0;

	while (1) {
		bytes = read(fd, buf, SECTOR_SIZE);
		printf("size of Tar: %d bytes\n", bytes);
		assert(bytes == SECTOR_SIZE); /* size of a TAR file
					       * must be multiple of 512
					       */
		if (buf[0] == 0) {
			if (i == 0)
			 	printf("    need not unpack the file.\n");
			break;
		}
		i++;

		struct posix_tar_header * phdr = (struct posix_tar_header *)buf;

		/* calculate the file size */
		char * p = phdr->size;
		int f_len = 0;
		while (*p)
			f_len = (f_len * 8) + (*p++ - '0'); /* octal */

		int bytes_left = f_len;
		int fdout = open(phdr->name, O_CREAT | O_RDWR | O_TRUNC);
		if (fdout == -1) {
			printf("    failed to extract file: %s\n", phdr->name);
			printf(" aborted]\n");
			close(fd);
			return;
		}
		printf("    %s\n", phdr->name);
		strcpy(temp_filename, phdr->name);

		while (bytes_left) {
			int iobytes = min(chunk, bytes_left);
			read(fd, buf,
			     ((iobytes - 1) / SECTOR_SIZE + 1) * SECTOR_SIZE);
			bytes = write(fdout, buf, iobytes);
			assert(bytes == iobytes);
			bytes_left -= iobytes;
		}
		close(fdout);

	// if (i) {
	// 	lseek(fd, 0, SEEK_SET);
	// 	buf[0] = 0;
	// 	bytes = write(fd, buf, 1);
	// 	assert(bytes == 1);
	// }
		if (STATIC_CHECK) {//计算每一个文件的原始偶校验值
            int fd_check = open(temp_filename, O_RDWR);
            if (fd_check == -1) {
                printf("open check file failed\n");
            }
            Check check;
			check.check_value = 0;  // 初始化校验和为0，开始计算校验和
			int bytes_get = 1;
			char byte128[128];  // 定义一个128字节的缓冲区
			strcpy(check.name, temp_filename);  

			while (bytes_get) {  // 循环读取文件直到读取完
				bytes_get = read(fd_check, &byte128, sizeof(byte128));  // 每次读取128字节
				for (int i = 0; i < bytes_get; i++) {
					check.check_value ^= calculate_even_parity(byte128[i]);  // 对每个字节进行偶校验的异或操作
				}
			}
			write(check_file_fd, &check, sizeof(check));  // 写入最终的校验结果
			close(fd_check);
		}
	}
	
	if (i) {
		lseek(fd, 0, SEEK_SET);
		buf[0] = 0;
		bytes = write(fd, buf, 1);
		assert(bytes == 1);
	}
	
	if (STATIC_CHECK) {
        close(check_file_fd);
    }

	close(fd);

	printf(" done, %d files extracted]\n", i);
}

int check_valid(char *filename) 
{
    int check_fd = open("check", O_RDWR);
    if (check_fd == -1) {
        printf("Error: Unable to open check file\n");
        return 0;
    }

    Check check;
	int origin_value ;
    int flag = 0;
    // 在 check 中查找该文件的校验值
    while (read(check_fd, &check, sizeof(check)) > 0) {
        if (strcmp(check.name, filename) == 0) {
			origin_value = check.check_value;
            flag = 1;
            break;  // 找到匹配的文件
        }
    }
    close(check_fd);

    if (flag == 0) {
        printf("Sorry, %s not exist in the system\n", filename);
        return 0;
    } else {
        // 校验当前文件的校验值
        int crt_file_fd = open(filename, O_RDWR);
        if (crt_file_fd == -1) {
            printf("Error: Unable to open file %s\n", filename);
            return 0;
        }

        int calculated_checkvalue = 0;  // 初始化计算的校验和为 0
        char temp_byte[128];          // 存放读取的文件内容
        int bytes_get = 1;
        
        // 读取文件并计算校验值
        while (bytes_get) {
            bytes_get = read(crt_file_fd, &temp_byte, sizeof(temp_byte)); // 读取文件
            for (int i = 0; i < bytes_get; i++) {
                calculated_checkvalue ^= calculate_even_parity(temp_byte[i]);  // 计算偶校验
            }
        }

        close(crt_file_fd);

        // 比较计算出的校验值和存储的原始校验值
		printf("current_checkvalue = %d\norigin_value = %d\n",calculated_checkvalue, origin_value);
        if (calculated_checkvalue == origin_value) {
            printf("File is valid! Check_value right.\n");
            return 1;
        } else {
            printf("Sorry, %s has been modified, can not execv.\n", filename);
            return 0;
        }
    }
}


/*****************************************************************************
 *                                shabby_shell
 *****************************************************************************/
/**
 * A very very simple shell.
 * 
 * @param tty_name  TTY file name.
 *****************************************************************************/

#define MAX_SHELL_PROC 5 //最多指令条数    
#define MAX_SHELL_PROC_STACK 128 //一次输入的最大长度
   

void shabby_shell(const char * tty_name)
{
	char* multi_argv[MAX_SHELL_PROC][MAX_SHELL_PROC_STACK];  //multi_argv保存二维字符串数组
    int fd_stdin  = open(tty_name, O_RDWR);
    assert(fd_stdin  == 0);
    int fd_stdout = open(tty_name, O_RDWR);
    assert(fd_stdout == 1);

    char rdbuf[128];

    while (1) {
        write(1, "$ ", 2);
        int r = read(0, rdbuf, 70);
        rdbuf[r] = 0;
		// printf("shell:log_fd:%d\n", log_fd);
		SYSLOG("{tasktty} command:%s\n", rdbuf);

        int argc = 0;
        char * argv[PROC_ORIGIN_STACK];
        char * p = rdbuf;
        char * s;
        int word = 0;
        char ch;
        do {
            ch = *p;
            if (*p != ' ' && *p != 0 && !word) {
                s = p;
                word = 1;
            }
            if ((*p == ' ' || *p == 0) && word) {
                word = 0;
                argv[argc++] = s;
                *p = 0;
            }
            p++;
        } while(ch);
        argv[argc] = 0;

		// for(int gg=0; gg<3; ++gg) {
		// 	printl("%s ", argv[gg]);
		// }
		// printl("\n");
		// printl("argc:%d\n", argc);

        // 初始化变量
        int num_proc = 1; // 表示有多少个命令  
		int sec_count = 0; // 当前命令的参数计数 
		int error = 0; // 标记命令是否出错 

        // 将命令和参数分配到 multi_argv 中
        for (int i = 0; i < argc; i++) {
            if (strcmp(argv[i], "&") != 0) {
                multi_argv[num_proc - 1][sec_count++] = argv[i];
            } else {
                multi_argv[num_proc - 1][sec_count] = 0; 	
                num_proc++;
                sec_count = 0;
                if (num_proc > MAX_SHELL_PROC) {
                    error = 1;
                    printf("Too many commands!\n");
                    break;
                }
            }
        }

        // 执行命令
        if (!error)  
		{  
			int fd = 0;
			for (int i = 0; i < num_proc; i++)  
			{  
				if(strcmp(multi_argv[i][0], "set") != 0) {
					fd = open(multi_argv[i][0], O_RDWR);  
				}
				else {
					int set_flag = 0;
					if(strcmp(multi_argv[i][1], "open") == 0) {
						set_flag = 1;
					}
					if(strcmp(multi_argv[i][2], "tty") == 0) {
						LOG_FLAGS[0] = set_flag;
					} else if (strcmp(multi_argv[i][2], "fs") == 0) {
						LOG_FLAGS[1] = set_flag; 
					} else if (strcmp(multi_argv[i][2], "mm") == 0) {
						LOG_FLAGS[2] = set_flag;
					} else if (strcmp(multi_argv[i][2], "device") == 0) {
						LOG_FLAGS[3] = set_flag;
					} else {
						set_flag = -1;
					}
					if(set_flag == -1){
						SYSLOG("{tasklog} Fail to Changed Log Parameter\n");
					} else {
						SYSLOG("{tasklog} Log Parameter Changed\n");
					}
					continue;
				}
				if (fd == -1) {  
					if (rdbuf[0]) {  
						write(1, "{", 1);  
						write(1, rdbuf, r);  
						write(1, "}\n", 2);  
					}  
				}  
				else {  
					//printl("shell_fd:%d\n", log_fd);
					int tmp_fd = fd;
					close(fd);  
					int pid = fork();
					
					if (pid != 0) { /* parent */  
						int s;  
						wait(&s); 
						if(strcmp(multi_argv[i][0], "open_f") == 0) {
							SYSLOG("{task_fs} type:WRITE filename:%s\n", multi_argv[i][1]);
						}
						if(strcmp(multi_argv[i][0], "echo") == 0) {
							SYSLOG("{task_fs} type:WRITE filename:%s\n", multi_argv[i][2]);
						}
						SYSLOG("{task_mm} type:EXIT pid:%d\n", pid);
						SYSLOG("{task_fs} type:CLOSE fd:%d\n", tmp_fd);
					}  
					else {  /* child */ 
						SYSLOG("{task_fs} type:OPEN filename:%s\n", multi_argv[i][0]); 
						SYSLOG("{task_mm} type:FORK\n");
						if(strcmp(multi_argv[i][0], "open_f") == 0) {
							SYSLOG("{task_fs} type:READ filename_read:%s\n", multi_argv[i][1]);
						}
						// 检查文件是否被修改
						if ((!STATIC_CHECK) || check_valid(multi_argv[i][0]) == 1) 
						execv(multi_argv[i][0], multi_argv[i]);  
					}
					
				}  
			}  
		}  
    }

    close(1);
    close(0);
}



/*****************************************************************************
 *                                Init
 *****************************************************************************/
/**
 * The hen.
 * 
 *****************************************************************************/
void Init()
{
	int fd_stdin  = open("/dev_tty0", O_RDWR);
	assert(fd_stdin  == 0);
	int fd_stdout = open("/dev_tty0", O_RDWR);
	assert(fd_stdout == 1);

	printf("Init() is running ...\n");

	/* extract `cmd.tar' */
	untar("/cmd.tar");
			

	char * tty_list[] = {"/dev_tty1", "/dev_tty2"};

	int i;
	for (i = 0; i < sizeof(tty_list) / sizeof(tty_list[0]); i++) {
		int pid = fork();
		if (pid != 0) { /* parent process */
			printf("[parent is running, child pid:%d]\n", pid);
		}
		else {	/* child process */
			printf("[child is running, pid:%d]\n", getpid());
			close(fd_stdin);
			close(fd_stdout);
			
			shabby_shell(tty_list[i]);
			assert(0);
		}
	}

	while (1) {
		int s;
		int child = wait(&s);
		printf("child (%d) exited with status: %d.\n", child, s);
	}

	assert(0);
}


/*======================================================================*
                               TestA
 *======================================================================*/
void TestA()
{
	for(;;);
}

/*======================================================================*
                               TestB
 *======================================================================*/
void TestB()
{
	for(;;);
}

/*======================================================================*
                               TestB
 *======================================================================*/
void TestC()
{
	for(;;);
}

/*****************************************************************************
 *                                panic
 *****************************************************************************/
PUBLIC void panic(const char *fmt, ...)
{
	int i;
	char buf[256];

	/* 4 is the size of fmt in the stack */
	va_list arg = (va_list)((char*)&fmt + 4);

	i = vsprintf(buf, fmt, arg);

	printl("%c !!panic!! %s", MAG_CH_PANIC, buf);

	/* should never arrive here */
	__asm__ __volatile__("ud2");
}

