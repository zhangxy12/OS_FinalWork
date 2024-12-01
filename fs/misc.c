/*************************************************************************//**
 *****************************************************************************
 * @file   misc.c
 * @brief  
 * @author Forrest Y. Yu
 * @date   2008
 *****************************************************************************
 *****************************************************************************/

/* Orange'S FS */

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
#include "hd.h"
#include "fs.h"

/*****************************************************************************
 *                                do_stat
 *************************************************************************//**
 * Perform the stat() syscall.
 * 
 * @return  On success, zero is returned. On error, -1 is returned.
 *****************************************************************************/
PUBLIC int do_stat()
{
	char pathname[MAX_PATH]; /* parameter from the caller */
	char filename[MAX_PATH]; /* directory has been stipped */

	/* get parameters from the message */
	int name_len = fs_msg.NAME_LEN;	/* length of filename */
	int src = fs_msg.source;	/* caller proc nr. */
	assert(name_len < MAX_PATH);
	phys_copy((void*)va2la(TASK_FS, pathname),    /* to   */
		  (void*)va2la(src, fs_msg.PATHNAME), /* from */
		  name_len);
	pathname[name_len] = 0;	/* terminate the string */

	int inode_nr = search_file(pathname);
	if (inode_nr == INVALID_INODE) {	/* file not found */
		printl("{FS} FS::do_stat():: search_file() returns "
		       "invalid inode: %s\n", pathname);
		return -1;
	}

	struct inode * pin = 0;

	struct inode * dir_inode;
	if (strip_path(filename, pathname, &dir_inode) != 0) {
		/* theoretically never fail here
		 * (it would have failed earlier when
		 *  search_file() was called)
		 */
		assert(0);
	}
	pin = get_inode(dir_inode->i_dev, inode_nr);

	struct stat s;		/* the thing requested */
	s.st_dev = pin->i_dev;
	s.st_ino = pin->i_num;
	s.st_mode= pin->i_mode;
	s.st_rdev= is_special(pin->i_mode) ? pin->i_start_sect : NO_DEV;
	s.st_size= pin->i_size;

	put_inode(pin);

	phys_copy((void*)va2la(src, fs_msg.BUF), /* to   */
		  (void*)va2la(TASK_FS, &s),	 /* from */
		  sizeof(struct stat));

	return 0;
}

/*****************************************************************************
 *                                search_file
 *****************************************************************************/
/**
 * Search the file and return the inode_nr.
 *
 * @param[in] path The full path of the file to search.
 * @return         Ptr to the i-node of the file if successful, otherwise zero.
 * 
 * @see open()
 * @see do_open()
 *****************************************************************************/
PUBLIC int search_file(char * path)
{
	int i, j;

	char filename[MAX_PATH];
	memset(filename, 0, MAX_FILENAME_LEN);
	struct inode * dir_inode;
	if (strip_path(filename, path, &dir_inode) != 0)
		return 0;

	if (filename[0] == 0)	/* path: "/" */
		return dir_inode->i_num;

	/**
	 * Search the dir for the file.
	 */
	int dir_blk0_nr = dir_inode->i_start_sect;
	int nr_dir_blks = (dir_inode->i_size + SECTOR_SIZE - 1) / SECTOR_SIZE;
	int nr_dir_entries =
	  dir_inode->i_size / DIR_ENTRY_SIZE; /**
					       * including unused slots
					       * (the file has been deleted
					       * but the slot is still there)
					       */
	int m = 0;
	struct dir_entry * pde;
	for (i = 0; i < nr_dir_blks; i++) {
		RD_SECT(dir_inode->i_dev, dir_blk0_nr + i);
		pde = (struct dir_entry *)fsbuf;
		for (j = 0; j < SECTOR_SIZE / DIR_ENTRY_SIZE; j++,pde++) {
			if (memcmp(filename, pde->name, MAX_FILENAME_LEN) == 0)
				return pde->inode_nr;
			if (++m > nr_dir_entries)
				break;
		}
		if (m > nr_dir_entries) /* all entries have been iterated */
			break;
	}

	/* file not found */
	return 0;
}

/*****************************************************************************
 *                                strip_path
 *****************************************************************************/
/**
 * Get the basename from the fullpath.
 *
 * In Orange'S FS v1.0, all files are stored in the root directory.
 * There is no sub-folder thing.
 *
 * This routine should be called at the very beginning of file operations
 * such as open(), read() and write(). It accepts the full path and returns
 * two things: the basename and a ptr of the root dir's i-node.
 *
 * e.g. After stip_path(filename, "/blah", ppinode) finishes, we get:
 *      - filename: "blah"
 *      - *ppinode: root_inode
 *      - ret val:  0 (successful)
 *
 * Currently an acceptable pathname should begin with at most one `/'
 * preceding a filename.
 *
 * Filenames may contain any character except '/' and '\\0'.
 *
 * @param[out] filename The string for the result.
 * @param[in]  pathname The full pathname.
 * @param[out] ppinode  The ptr of the dir's inode will be stored here.
 * 
 * @return Zero if success, otherwise the pathname is not valid.
 *****************************************************************************/
PUBLIC int strip_path(char * filename, const char * pathname,
		      struct inode** ppinode)
{
	const char * s = pathname;
	char * t = filename;

	if (s == 0)
		return -1;

	if (*s == '/')
		s++;

	while (*s) {		/* check each character */
		if (*s == '/')
			return -1;
		*t++ = *s++;
		/* if filename is too long, just truncate it */
		if (t - filename >= MAX_FILENAME_LEN)
			break;
	}
	*t = 0;

	*ppinode = root_inode;

	return 0;
}

/*****************************************************************************
 *                                do_open_dir
 *****************************************************************************/
/**
 * Open a directory, read its contents, and store the file names.
 *
 * This function searches for the directory specified by the `dir` path, reads
 * its entries, and copies the names of the files in the directory into the 
 * `dir` buffer. The directory's inode is located, and the directory is read
 * sector by sector.
 *
 * @param[in]  dir The buffer that will hold the names of the directory files.
 * @return     0 if the directory is successfully opened and processed, 
 *             otherwise 0 (indicating failure).
 *
 * ls
 *****************************************************************************/
PUBLIC int do_open_dir() {
    struct inode* dir_inode;
    char filename[MAX_PATH];
    char* dir = fs_msg.pBUF;
    int pointer = 0;

    printl("here : %s\n", dir);
    memset(filename, 0, MAX_FILENAME_LEN);
    if (strip_path(filename, dir, &dir_inode) != 0) {
        return 0;
    }


    int dir_blk0_nr = dir_inode->i_start_sect;
    int nr_dir_blks = (dir_inode->i_size + SECTOR_SIZE - 1) / SECTOR_SIZE;
    int nr_dir_entries =
        dir_inode->i_size / DIR_ENTRY_SIZE; /**
                                             * including unused slots
                                             * (the file has been deleted
                                             * but the slot is still there)
                                             */
    struct dir_entry* pde;
    int i, j;
    for (i = 0; i < nr_dir_blks; i++) {
        RD_SECT(dir_inode->i_dev, dir_blk0_nr + i);
        pde = (struct dir_entry*)fsbuf;
        for (j = 0; j < SECTOR_SIZE / DIR_ENTRY_SIZE; j++, pde++) {
            // printl("%s  ", pde->name);
            dir[pointer] = ' ';
            pointer += 1;
            memcpy(dir + pointer, pde->name, strlen(pde->name));
            pointer += strlen(pde->name);
        }
    }
    // printl("after for : %s\n", dir);
    return (void*)0;
}

/*****************************************************************************
 *                                do_open_dir_l
 *****************************************************************************/
/**
 * Open a directory, read its entries, and store detailed file information 
 * (such as name, inode properties, permissions, size, and more) into a 
 * buffer for later use.
 *
 * This function searches for the directory specified by the path, reads its 
 * entries, and for each file in the directory, retrieves and formats detailed 
 * inode information, including the file's name, permissions, size, sector 
 * information, and more. It stores the processed data into a static buffer 
 * and then copies the result into `fs_msg.lBUF`.
 *
 * @param[in] dir The buffer that will hold the formatted directory and file 
 *                information.
 * 
 * @return     0 if the directory is successfully processed and the information 
 *             is stored, otherwise 0 (indicating failure).
 * 
 *ls_l
 *****************************************************************************/
PUBLIC int do_open_dir_l() {
    struct inode* dir_inode;
    char filename[MAX_PATH_L];
    char dir_static[MAX_PATH_L];  // 静态缓冲区，存储目录信息
    char* dir = fs_msg.lBUF;      // 目标缓冲区，复制内容到 fs_msg.lBUF
    int pointer = 0;
    char temp[128];  // 临时存储转换后的文件属性

    printl("Opening directory: %s\n", dir);
    memset(filename, 0, MAX_FILENAME_LEN);

    // 获取目录的 inode 信息
    if (strip_path(filename, dir, &dir_inode) != 0) {
        printl("Failed to get inode from path\n");
        return 0;
    }

    // 获取目录块号信息
    int dir_blk0_nr = dir_inode->i_start_sect;
    int nr_dir_blks = (dir_inode->i_size + SECTOR_SIZE - 1) / SECTOR_SIZE;
    struct dir_entry* pde;
	int nr_dir_entries =
        dir_inode->i_size / DIR_ENTRY_SIZE; /**
                                             * including unused slots
                                             * (the file has been deleted
                                             * but the slot is still there)
                                             */
    int i, j;

    // 清空静态缓冲区
    memset(dir_static, 0, sizeof(dir_static));
    int static_pointer = 0;  // 用于跟踪静态缓冲区的位置

    // 遍历目录块
    for (i = 0; i < nr_dir_blks; i++) {
        RD_SECT(dir_inode->i_dev, dir_blk0_nr + i);
        pde = (struct dir_entry*)fsbuf;

        // 遍历目录中的所有条目
        for (j = 0; j < SECTOR_SIZE / DIR_ENTRY_SIZE; j++, pde++) {
            // 跳过空目录项
            if (pde->name[0] == '\0') {
                continue;
            }

            // 打印文件名
            if (static_pointer >= sizeof(dir_static) - 1) {
                printl("Buffer overflow detected.\n");
                return 0;
            }
            dir_static[static_pointer++] = ' ';  // 文件名之前的空格
            memcpy(dir_static + static_pointer, pde->name, strlen(pde->name));
            static_pointer += strlen(pde->name);

            // 获取文件 inode 信息
            struct inode* file_inode = get_inode(dir_inode->i_dev, pde->inode_nr);
            if (!file_inode) {
                printl("Error retrieving inode for file: %s\n", pde->name);
                continue;
            }
			memset(temp, 0, sizeof(temp));
            // 获取文件的权限模式（i_mode）
            itoa(temp, file_inode->i_mode);  // 转换为十六进制字符串
            if (static_pointer + strlen(temp) + 1 >= sizeof(dir_static)) {
                printl("Buffer overflow detected.\n");
                return 0;
            }
            dir_static[static_pointer++] = ' ';
            memcpy(dir_static + static_pointer, temp, strlen(temp));  // 文件权限模式（十六进制）
            static_pointer += strlen(temp);

			memset(temp, 0, sizeof(temp));
            // 获取文件的大小（i_size）并转换为十六进制格式
            itoa(temp, file_inode->i_size);  // 转换为十六进制字符串
            if (static_pointer + strlen(temp) + 1 >= sizeof(dir_static)) {
                printl("Buffer overflow detected.\n");
                return 0;
            }
            dir_static[static_pointer++] = ' ';
            memcpy(dir_static + static_pointer, temp, strlen(temp));  // 文件大小（十六进制）
            static_pointer += strlen(temp);

            // 获取文件的起始扇区（i_start_sect）并转换为十六进制格式
			memset(temp, 0, sizeof(temp));
            itoa(temp, file_inode->i_start_sect);
            if (static_pointer + strlen(temp) + 1 >= sizeof(dir_static)) {
                printl("Buffer overflow detected.\n");
                return 0;
            }
            dir_static[static_pointer++] = ' ';
            memcpy(dir_static + static_pointer, temp, strlen(temp));  // 文件起始扇区（十六进制）
            static_pointer += strlen(temp);

            // 获取文件的占用扇区数（i_nr_sects）并转换为十六进制格式
			memset(temp, 0, sizeof(temp));
            itoa(temp, file_inode->i_nr_sects);
            if (static_pointer + strlen(temp) + 1 >= sizeof(dir_static)) {
                printl("Buffer overflow detected.\n");
                return 0;
            }
            dir_static[static_pointer++] = ' ';
            memcpy(dir_static + static_pointer, temp, strlen(temp));  // 文件占用扇区数（十六进制）
            static_pointer += strlen(temp);

            // 获取文件设备编号（i_dev）
			memset(temp, 0, sizeof(temp));
            itoa(temp, file_inode->i_dev);
            if (static_pointer + strlen(temp) + 1 >= sizeof(dir_static)) {
                printl("Buffer overflow detected.\n");
                return 0;
            }
            dir_static[static_pointer++] = ' ';
            memcpy(dir_static + static_pointer, temp, strlen(temp));  // 文件设备编号（十六进制）
            static_pointer += strlen(temp);

            // 获取文件引用计数（i_cnt）
			memset(temp, 0, sizeof(temp));
            itoa(temp, file_inode->i_cnt);
            if (static_pointer + strlen(temp) + 1 >= sizeof(dir_static)) {
                printl("Buffer overflow detected.\n");
                return 0;
            }
            dir_static[static_pointer++] = ' ';
            memcpy(dir_static + static_pointer, temp, strlen(temp));  // 文件引用计数（十六进制）
            static_pointer += strlen(temp);

            // 获取文件 inode 编号（i_num）
			memset(temp, 0, sizeof(temp));
            itoa(temp, file_inode->i_num);
            if (static_pointer + strlen(temp) + 1 >= sizeof(dir_static)) {
                printl("Buffer overflow detected.\n");
                return 0;
            }
            dir_static[static_pointer++] = ' ';
            memcpy(dir_static + static_pointer, temp, strlen(temp));  // 文件 inode 编号（十六进制）
            static_pointer += strlen(temp);

            // 换行符分隔每个文件
            dir_static[static_pointer++] = '\n';
        }
    }

    // 确保静态缓冲区末尾的字符串终止符
    dir_static[static_pointer] = '\0';

    // 最后将静态缓冲区的内容复制到 fs_msg.lBUF 中
    memcpy(fs_msg.lBUF, dir_static, static_pointer + 1);

    return (void*)0;
}
