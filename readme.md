**任务二**

利用当前OrangeS所提供的系统调用和API，扩展Shell命令

1. 支持进程管理，包括

   - 列出当前运行的进程

     指令：ps

   - 终止指定的进程

     指令：kill  <process_name>

2. 支持文件管理，包括

   - 列出当前目录的文件，以及文件的相关属性信息

     列出目录文件名：ls

     列出文件所有属性：ls_l 

   - 创建新文件

     指令：touch  <file_name>

   - 打开或编辑指定文件（如果是可执行文件，则运行，如果是文本文件，则打开可编辑）

     指令：open_f  <file_name>

   - 删除指定文件

     指令：rm  <file_name>

3. 支持在同一个TTY上，可并发运行多个shell任务

   指令 & 指令 & 。（最多支持5个同时）