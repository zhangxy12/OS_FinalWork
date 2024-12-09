[SECTION .data]	 ; 数据段
_BitMap :  	times  32  db  0xff  ;占用(0  ~  0x1ffff)
			times  32  db  0x0  ;空闲	

BitMapLen 	equ  $  -   $$

PhysicalAddr		db "PhysicalAddr:",0
LinearAddr		db "LinearAddr:",0
dwDispPos			dd	(80 * 20 + 0) * 2	; 屏幕第 6 行, 第 0 列。
szReturn			db	0Ah, 0

global alloc_page
global free_page

[SECTION .text]
alloc_page:                               ; arg: eax : virtual address
	; return ebx : physical address  
	push ds
	push es
	
	mov ecx, eax             ; 复制虚拟地址
	
	; 获取虚拟地址的页目录项
	and  eax, 0xffc00000
	shr  eax, 20
	mov edi, cr3             ; edi = cr3 
	and edi, 0xfffff000		; (cr3高20位是页目录基地址)
	add edi, eax             ; edi = 页目录项地址
	mov ebx, [edi]           ; ebx = 页目录项内容
	
	test ebx, 0x00000001     ; 检查页目录是否存在
	jnz .pde_exist            ; 如果存在，跳转

	; 如果页目录项不存在，创建页表
	
	mov ebx, cr3             
	mov ebx, [ebx]            ; 页目录
	and ebx, 0xfffff000			;页表基地址,也即是页目录的内容
	call alloc_a_4k_page
	or eax, 0x00000007
	mov [edi], eax; 更新页目录项
	
.pde_exist:
	mov ebx,[edi]
	and ebx, 0xfffff000      ; 获取页表基地址
	mov eax, ecx             ; 复制虚拟地址
	and eax, 0x003ff000
	shr eax, 10
    add ebx, eax             ; 计算页表项地址
	
	call alloc_a_4k_page
	or eax, 0x00000007
	mov [ebx], eax
	
.pte_exist:	
	and eax, 0xfffff000      ; 清除 PTE 中的标志位
    mov ebx, eax            ; 将物理地址放入 ebx
	
	; 获取虚拟地址的低12位并合并
    mov edx, ecx             ; edx = virtual address
    and edx, 0x00000FFF      ; 获取低12位偏移
    add ebx, edx              ; 将偏移加到物理地址上
	
	pop es
	pop ds

	; 输出线性地址和物理地址
	push	LinearAddr
	call	DispStr
	add		esp, 4
	
    mov     eax, ecx
    push    eax
	call    DispInt
	pop		eax
	
	push	PhysicalAddr
	call	DispStr
	add		esp, 4
    push    ebx                    ; 保存物理地址
	call    DispInt
	pop 	ebx
	ret
	
alloc_a_4k_page:
	push  ds
	push  es

	; 查找可用的物理地址
	xor esi, esi             ; 从0开始查找位图
.check_bitmap:
	bts  [_BitMap], esi       ; 检查位图
	jnc  .found_page          ; 如果该位为0，找到可分配的页面
	inc  esi
	cmp  esi, BitMapLen * 8		;1 BYTE = 8 bits
	jl   .check_bitmap

	; 没有可用物理页面
	hlt

.found_page:
	shl  esi, 12              ; 转换为物理地址
	mov eax, esi
	pop  es
	pop  ds
	ret	

; ---------------------------------------------------------------------------
free_page:
    push ds
    push es

    mov ecx, eax                ; ecx 保存虚拟地址
    mov edx, eax                ; edx 保存虚拟地址
    shr eax, 22                 ; 页目录索引
    mov ebx, cr3                ; 获取页目录基址
    and ebx, 0xfffff000         ; 保留高20位
    shl eax, 2                  ; 页目录项偏移
    add ebx, eax                ; 获取页目录项地址
    mov eax, [ebx]              ; 读取页目录项内容
    and eax, 0xfffffff8         ; 清除存在位
    mov [ebx], eax              ; 更新页目录项

    ; 清除页表项的存在位
    shr edx, 12                 ; 获取页表索引
    and edx, 0x03FF             ; 限制为10位
    shl edx, 2                  ; 页表项偏移
    mov ebx, cr3                ; 获取页目录基址
    add ebx, eax                ; 获取页表基址
    mov eax, [ebx]              ; 读取页表项内容
    and eax, 0xfffffff8         ; 清除存在位
    mov [ebx], eax              ; 更新页表项

    ; 计算物理地址并更新位图
    shr ecx, 12                 ; 获取物理地址
    and ecx, 0xfffff000         ; 清除标志位
    shr ecx, 12                 ; 获取页帧号
    btr [_BitMap], ecx           ; 更新位图

    pop es
    pop ds
    ret
; ------------------------------------------------------------------------
; 显示 AL 中的数字
; ------------------------------------------------------------------------
DispAL:
	push	ecx
	push	edx
	push	edi

	mov	edi, [dwDispPos]

	mov	ah, 0Fh			; 0000b: 黑底    1111b: 白字
	mov	dl, al
	shr	al, 4
	mov	ecx, 2
.begin:
	and	al, 01111b
	cmp	al, 9
	ja	.1
	add	al, '0'
	jmp	.2
.1:
	sub	al, 0Ah
	add	al, 'A'
.2:
	mov	[gs:edi], ax
	add	edi, 2

	mov	al, dl
	loop	.begin
	;add	edi, 2

	mov	[dwDispPos], edi

	pop	edi
	pop	edx
	pop	ecx

	ret
; DispAL 结束-------------------------------------------------------------


; ------------------------------------------------------------------------
; 显示一个整形数
; ------------------------------------------------------------------------
DispInt:
	mov	eax, [esp + 4]
	shr	eax, 24
	call	DispAL

	mov	eax, [esp + 4]
	shr	eax, 16
	call	DispAL

	mov	eax, [esp + 4]
	shr	eax, 8
	call	DispAL

	mov	eax, [esp + 4]
	call	DispAL

	mov	ah, 07h			; 0000b: 黑底    0111b: 灰字
	mov	al, 'h'
	push	edi
	mov	edi, [dwDispPos]
	mov	[gs:edi], ax
	add	edi, 4
	mov	[dwDispPos], edi
	pop	edi

	ret
; DispInt 结束------------------------------------------------------------

; ------------------------------------------------------------------------
; 显示一个字符串
; ------------------------------------------------------------------------
DispStr:
	push	ebp
	mov	ebp, esp
	push	ebx
	push	esi
	push	edi

	mov	esi, [ebp + 8]	; pszInfo
	mov	edi, [dwDispPos]
	mov	ah, 0Fh
.1:
	lodsb
	test	al, al
	jz	.2
	cmp	al, 0Ah	; 是回车吗?
	jnz	.3
	push	eax
	mov	eax, edi
	mov	bl, 160
	div	bl
	and	eax, 0FFh
	inc	eax
	mov	bl, 160
	mul	bl
	mov	edi, eax
	pop	eax
	jmp	.1
.3:
	mov	[gs:edi], ax
	add	edi, 2
	jmp	.1

.2:
	mov	[dwDispPos], edi

	pop	edi
	pop	esi
	pop	ebx
	pop	ebp
	ret
; DispStr 结束------------------------------------------------------------

; ------------------------------------------------------------------------
; 换行
; ------------------------------------------------------------------------
DispReturn:
	push	szReturn
	call	DispStr			;printf("\n");
	add	esp, 4

	ret
; DispReturn 结束---------------------------------------------------------
