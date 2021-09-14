;功能：IPL：启动程序装载器，装载启动程序
;1.读取512字节之后的内容
;2.重试5次，如果读取错误或者装载错误，则会重新retry，当计数器大于等于5时，跳转到error
;3.往后多读几个扇区,一共读取18个扇区
;4.在C0H0S18之后，就是C0H1S1，在次读入10个柱面，到此处，我们已经将软盘的前10和煮面全部装在进了内存中10*2*18*512=183320Bytes
; hello-os
; TAB=4
CYLS	EQU	10;		定义CYLS的大小是10

	ORG	0x7c00		; 启动去内容的装载位置0x00007c000-0x00007dff

; 以下是标准FAT12格式软盘的代码

	JMP		entry
	DB		0x90
	DB	"HELLOIPL"	; 启动区的名称可以是任意的字符串（8字节）
	DW	512		; 每个扇区（sector） 的大小（必须是512字节）
	DB	1		; 簇（cluster）的大小（必须是1个扇区）
	DW	1		; FAT的起始位置（一般从第一个扇区?始）
	DB	2		; FAT的个数（必须是2）
	DW	224		; 根目录的大小（一般设成224项）
	DW	2880		; 该磁盘的大小（必须是2280扇区）
	DB	0xf0		; 磁盘的种类（0xf0）
	DW	9		; FAT的长度（必须是9扇区）
	DW	18		; 1个磁道（track）有几个扇区（必须是18）
	DW	2		; 磁盘数（必须是2）
	DD	0		; 不使用分区，必须是0
	DD	2880		; 重写一次磁盘大小
	DB	0,0,0x29	; 意义不明，固定
	DD	0xffffffff	; 卷标号码
	DB	"HELLO-OS   "	; 磁的名称（11字节）
	DB	"FAT12   "	; 磁?格式名称（8字节）
	RESB	18		; 先空出18字节

; 程序核心		
entry:
	MOV AX,0		; 初始化寄存器
	MOV SS,AX
	MOV SP,0x7c00	;栈顶指针指向启动区装载地址
	MOV DS,AX
	MOV ES,AX

	MOV		AX,0x0820
	MOV		ES,AX
	MOV		CH,0			; 柱面0
	MOV		DH,0			; 磁头0
	MOV		CL,2			; 扇区2

readloop:
	MOV		SI,0			;记录失败次数的寄存器
	
retry:
	MOV		AH,0x02			; AH=0x02 : 读盘
	MOV		AL,1			; 1个扇区
	MOV		BX,0
	MOV		DL,0x00			; A驱动器
	INT		0x13			; 调用磁盘BIOS
	JNC		next				;没出错就跳到fin
	ADD 	SI,1			;出错读取不到时，SI加一
	CMP 	SI,5			;比较SI和5
	JAE 	error			;当SI>=5，跳转到error
	MOV		DL,0x00			; A驱动器
	INT		0x13			; 调用磁盘BIOS
	JMP 	retry

next:
	MOV 	AX,ES
	ADD		AX,0x0020
	MOV		ES,AX			;前面三条语句，就是将内存地址向后移动0x0200(512字节)，0x0020*16(ES*16+BX)
	ADD		CL,1			;向CL中加一，CL表示都那个扇区
	CMP		CL,18			;比较CL和18
	JBE		readloop		;CL小于等于18，则继续跳转到readloop，读取扇区	
	MOV		CL,1			; 扇区1
	ADD		DH,1			; 磁头加1，磁头1
	CMP 	DH,2			;
	JB		readloop		;DH<2,跳回readloop，到这里一个柱面读完
	MOV 	DH,0			;再次换成磁头0
	ADD		CH,1			;读取柱面1
	CMP 	CH,CYLS
	JB		readloop		;CH<CYLS(10),继续读取

;跳转到haribote.sys
	MOV		[0x0ff0],CH
	JMP		0xc200

fin:
    HLT                      ; 让CPU停止；等待指令
    JMP fin                  ; 无限循环

error:
	MOV SI,msg
putloop:
	MOV AL,[SI]
	ADD SI,1		; SI加1
	CMP AL,0
	JE fin
	MOV AH,0x0e		; 显示一个文字
	MOV BX,15		; 指定字符的颜色
	INT 0x10		; 调用显卡BIOS
	JMP putloop

msg:
	DB	0x0a, 0x0a		; 换行2次
	DB	"Welcome to Thanos'OS"
	;DB     "hello world!"
	DB	0x0a			; 换行
	DB	0

	RESB	0x7dfe-$		; 填写0x00，直到0x001fe

	DB	0x55, 0xaa
