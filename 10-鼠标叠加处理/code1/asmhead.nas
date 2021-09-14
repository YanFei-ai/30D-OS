; Thanos-os boot asm
; TAB=4

BOTPAK	EQU		0x00280000		; bootpack要copy道德目的位置
DSKCAC	EQU		0x00100000		; 启动区的目的位置
DSKCAC0	EQU		0x00008000		; 磁盘上的内容装载到了内存的0x8000地址，扇区一作为启动扇区 512KB

;BOOT INFO
CYLS    EQU     0x0ff0      ;承接ipl10.nas中的值，设定启动区
LEDS	EQU		0x0ff1
VMODE	EQU		0x0ff2		;关于颜色数目的信息，颜色的位数放在了0x0ff2
SCRNX	EQU		0x0ff4		;分辨率的X
SCRNY	EQU		0x0ff6		;分辨率的Y
VRAM	EQU		0x0ff8		;图像缓冲区的开始地址

    ORG		0xc200  ;执行磁盘影像上位于0x4200地址处的程序
    ;因为磁盘上的内容装载到了内存的0x8000地址，所以
    ;磁盘上0x4200对饮内存的0xc200

    MOV     AL,0x13			;VGA显卡
    MOV     AH,0x00
    INT     0x10
    MOV     BYTE[VMODE],8        ;记录画面模式
    MOV     WORD[SCRNX],320
    MOV     WORD[SCRNY],200
    MOV     DWORD[VRAM],0x000a0000   ;这个值是因为INT 10，表示这种画面模式下，VRAM是0xa000-0xffff的64k
                                    ;VRAM video RAM，每个地址都对应着画面上的像素

    ;用BIOS取得键盘上各种LED指示灯的状态
    MOV     AH,0x02
    INT     0x16                    ;keyboard BIOS
    MOV     [LEDS],AL

; PIC关闭一切中断
;	根据AT兼容机的规格，如果初始化PIC，
;	必须在CLI之前进行，否则有时会挂起
;	随后进行PIC的初始化

		MOV		AL,0xff
		OUT		0x21,AL
		NOP						; 如果连续执行OUT指令，有些电脑无法正常运行，NOP让CPU休息一个时钟长的时间
		OUT		0xa1,AL

		CLI						; 禁止CPU级别的中断
; 等同于
; io_out(PIC0_IMR, 0xff);	//禁止主PIC的全部中断
; io_out(PIC1_IMR, 0xff);	//禁止从PIC的全部中断
; io_cli();					//禁止CPU级别的中断

;为了让CPU能够访问1MB以上的内存空间， 设定A20GATE
		CALL	waitkbdout		;等同于wait_KBC_sendready,结构与init_keyboard相同，功能就是实现往键盘控制电路发送指令
		MOV		AL,0xd1
		OUT		0x64,AL
		CALL	waitkbdout
		MOV		AL,0xdf			; enable A20
		OUT		0x60,AL
		CALL	waitkbdout

; 切换保护模式

[INSTRSET "i486p"]				; 想要使用486指令的叙述，INSTRSET指令，为了能使用386后面的LGDT、EAX、CR0等关键字

		LGDT	[GDTR0]			; 设定临时GDT
		MOV		EAX,CR0
		AND		EAX,0x7fffffff	; 设定CR0的bit31为0，为了禁分页
		OR		EAX,0x00000001	; 设置CR0的bit0为1，为了切换到保护模式
		MOV		CR0,EAX			; set CR0 to start protected-mode and forbid page-mode by register EAX
		JMP		pipelineflush	; switch into protected-mode, and then JMP 
pipelineflush:
; value of all seg-registers from 0x0000 to 0x0008(gdt+1), but change CS last 
		MOV		AX,1*8			;  RW-seg 32bit
		MOV		DS,AX
		MOV		ES,AX
		MOV		FS,AX
		MOV		GS,AX
		MOV		SS,AX

; transfer bootpack
; copy bootpack.hrb to address 0x00280000, 
	; 从bootpack的地址开始的512KB呢欸容复制到0x00280000
		MOV		ESI,bootpack	; src
		MOV		EDI,BOTPAK		; dest
		MOV		ECX,512*1024/4
		CALL	memcpy

; 磁盘数据最终传送到它本来的位置去

; 首先是启动区开始
	;从0x7c00复制512字节到0x00100000
		MOV		ESI,0x7c00		; src
		MOV		EDI,DSKCAC		; dest
		MOV		ECX,512/4
		CALL	memcpy

; 所有剩下的
	;将始于0x00008200的磁盘内容，复制到ox00100200
		MOV		ESI,DSKCAC0+512	; src
		MOV		EDI,DSKCAC+512	; dest
		MOV		ECX,0
		MOV		CL,BYTE [CYLS]
		IMUL	ECX,512*18*2/4	; 从柱面数变为字节数要 /4
		SUB		ECX,512/4		; 减去IPL
		CALL	memcpy

; 必须由asmhead.nas完成的工作，至此全部完毕
; 以后就交友bootpack来完成

; bootpack的启动

		MOV		EBX,BOTPAK
		MOV		ECX,[EBX+16]	; bootpack.hrb之后的第16号地址，即0x11a8
		ADD		ECX,3			; ECX += 3;
		SHR		ECX,2			; ECX /= 4;
		JZ		skip			; 没有要传送的东西时
		MOV		ESI,[EBX+20]	; 转送源   ; bootpack.hrb之后的第20号地址，即0x10c8
		ADD		ESI,EBX
		MOV		EDI,[EBX+12]	; 转送目的地  ; bootpack.hrb之后的第12号地址，即0x00310000，通过二进制文件bootpack.hrb来确认
		CALL	memcpy
skip:
		MOV		ESP,[EBX+12]	; 栈初始值
		JMP		DWORD 2*8:0x0000001b

waitkbdout:
		IN		 AL,0x64
		AND		 AL,0x02	
		IN		 AL,0x60		; 空读（为了清空数据接收缓存区中的垃圾数据）
		JNZ		waitkbdout		; AND的结果如果不是0，就跳到waitkbdout
		RET

memcpy:
		MOV		EAX,[ESI]
		ADD		ESI,4
		MOV		[EDI],EAX
		ADD		EDI,4
		SUB		ECX,1
		JNZ		memcpy			; 减法运算的结果如果不是0，就跳到memcpy
		RET
; memcpy 复制内存的程序

		ALIGNB	16		; 一直添加DBO指令，知道实际合适位置，ALIGNB	16就是地址能被16整除时
						; 如果变迁GDT0的地址不是8的整数倍，向段寄存器复制的MOV指令会慢一些
GDT0:
		RESB	8							; NULL selector ，0号是NULL selector，不能在这里设定段
		DW		0xffff,0x0000,0x9200,0x00cf	; 可读写的段 32bit
		DW		0xffff,0x0000,0x9a28,0x0047	; 可执行的段 32bit bootpack用
; 段1，2实在bootpack中初始化
		DW		0
GDTR0:
		DW		8*3-1
		DD		GDT0

		ALIGNB	16
bootpack:
