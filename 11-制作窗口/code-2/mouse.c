#include "bootpack.h"

struct FIFO8 mousefifo;

void inthandler2c(int *esp)
/* 来自PS2鼠标的中断 */
{
	unsigned char data;
	io_out8(PIC1_OCW2, 0x64);	/* IRQ-12��t������PIC1�ɒʒm */
	io_out8(PIC0_OCW2, 0x62);	/* IRQ-02��t������PIC0�ɒʒm */
	data = io_in8(PORT_KEYDAT);
	fifo8_put(&mousefifo, data);
	/*
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	boxfill8(binfo->vram, binfo->scrnx, COL8_000000, 0, 0, 32 * 8 - 1, 15);
	putfonts8_asc(binfo->vram, binfo->scrnx, 0, 0, COL8_FFFFFF, "INT 2C (IRQ-12) : PS/2 mouse");
	for (;;) {
		io_hlt();
	}
	*/
}

//激活鼠标
void enable_mouse(void)
{
	/* 激活鼠标 */
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_SENDTO_MOUSE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, MOUSECMD_ENABLE);
	return; /* 顺利的话，键盘控制器会返回ACK（0xfa） */
}


//接受鼠标数据的函数，对传入的结构体进行赋值,并让鼠标移动起来
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat)
{
	if (mdec->phase == 0) {
		//还没有数据，等待鼠标0xfa
		if (dat == 0xfa) {
			mdec->phase = 1;
		}
		return 0;
	}
	if (mdec->phase == 1) {
		//等待鼠标的第一字节
		if ((dat & 0xc8) == 0x08) {
			//如果第一字节正确
			mdec->buf[0] = dat;
			mdec->phase = 2;
		}
		return 0;
	}
	if(mdec->phase == 2){
		//等待鼠标的第二字节
		mdec->buf[1] = dat;
		mdec->phase = 3;
		return 0;
	}
	if(mdec->phase == 3){
		//等待鼠标的第三字节
		mdec->buf[2] = dat;
		mdec->phase = 1;
		mdec->btn = mdec->buf[0] & 0x07;		//保存按键的状态，鼠标键的状态只保留在buf[0]的低3位
		mdec->x = mdec->buf[1];		//x方向上的移动
		mdec->y = mdec->buf[2];		//y方向上的移动

		//x,y直接使用buf[0][1]的数据，但是需要使用第一字节对鼠标移动有放映的几位，将x和y的第8位及以后都设置成1
		if((mdec->buf[0] & 0x10) != 0){
			mdec->x |= 0xffffff00;
		}
		if((mdec->buf[0] & 0x20) != 0){
			mdec->y |= 0xffffff00;
		}
		mdec->y = ~ mdec->y;    //鼠标的y方向与画面函数相反
		return 1;
	}
	return -1;	
}
