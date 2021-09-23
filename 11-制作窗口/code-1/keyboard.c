#include "bootpack.h"

struct FIFO8 keyfifo;

//鼠标是IRQ12，键盘是IRQ1
//因此编写用于INT2c、INT21的中断处理程序
void inthandler21(int *esp)
/* 来自PS2键盘的中断 ,按下一个键，会在屏幕上显示出信息*/
{
	//struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	unsigned char data;
	io_out8(PIC0_OCW2, 0x61);		//告诉PIC发生IRQ1中断，将0x60+IRQ号输出给OCW2
	data = io_in8(PORT_KEYDAT);		//从0x0060设备接收到的数据
	fifo8_put(&keyfifo, data);
	return ;
}

void wait_KBC_sendready(void)
{
	/* 等待键盘控制电路准备完毕 */
	for (;;) {
		if ((io_in8(PORT_KEYSTA) & KEYSTA_SEND_NOTREADY) == 0) {
			break;
		}
	}
	return;
}

void init_keyboard(void)
{
	/* 初始化键盘控制电路 */
	wait_KBC_sendready();
	io_out8(PORT_KEYCMD, KEYCMD_WRITE_MODE);
	wait_KBC_sendready();
	io_out8(PORT_KEYDAT, KBC_MODE);
	return;
}