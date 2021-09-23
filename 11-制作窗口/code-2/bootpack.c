#include "bootpack.h"

extern struct FIFO8 keyfifo ,mousefifo;

void HariMain(void)
{
	struct BOOTINFO *binfo = (struct BOOTINFO *) ADR_BOOTINFO;
	char s[40], keybuf[32], mousebuf[128];    //mcursor[256], 
	int mx, my;
	int i;
	//unsigned int mem;
	struct MOUSE_DEC mdec;
	struct MEMMAN * memman = (struct MEMMAN *)MEMMAN_ADDR ;
	unsigned int memtotal, counter = 0;
	struct SHTCTL *shtctl;
	struct SHEET *sht_back, *sht_mouse, *sht_win;		//背景图层，和鼠标图层
	unsigned char *buf_back, buf_mouse[256], *buf_win;

	//unsigned char data[4];		//存放int.c中接受的键盘输入数据
	/*字符A的图形显示
	static char font_A[16] = {
		0x00, 0x18, 0x18, 0x18, 0x18, 0x24, 0x24, 0x24,
		0x24, 0x7e, 0x42, 0x42, 0x42, 0xe7, 0x00, 0x00
	};*/

	init_gdtidt();
	init_pic();
	io_sti();
	fifo8_init(&keyfifo, 32, keybuf);
	fifo8_init(&mousefifo, 128, mousebuf);
	io_out8(PIC0_IMR, 0xf9); 
	io_out8(PIC1_IMR, 0xef); 

	init_keyboard();
	enable_mouse();
	memtotal = memtest(0x00400000, 0xbfffffff);		//用MB来表示结果
	//sprintf(s, "memory %dMB", memtotal);
	//putfonts8_asc(binfo->vram, binfo->scrnx, 0, 32, COL8_FFFFFF, s);
	memman_init(memman);
	memman_free(memman, 0x00001000, 0x0009e000); 
	memman_free(memman, 0x00400000, memtotal - 0x00400000);		/*一共释放两块内存*/


	init_palette();	//设置调色板
	shtctl = shtctl_init(memman, binfo->vram, binfo->scrnx, binfo->scrny);
	//两个图层，一个背景back，一个鼠标
	sht_back = sheet_alloc(shtctl);
	sht_mouse = sheet_alloc(shtctl);
	sht_win = sheet_alloc(shtctl);

	buf_back = (unsigned char*)memman_alloc_4k(memman,binfo->scrnx * binfo->scrny);
	buf_win  = (unsigned char*)memman_alloc_4k(memman,160*52);

	sheet_setbuf(sht_back, buf_back, binfo->scrnx, binfo->scrny, -1);
	sheet_setbuf(sht_mouse, buf_mouse, 16, 16, 99);
	sheet_setbuf(sht_win, buf_win, 160, 52, -1);

	init_screen8(buf_back, binfo->scrnx, binfo->scrny);
	init_mouse_cursor8(buf_mouse, 99);
	make_windows(buf_win, 160, 52, "counter");

	sheet_slide(sht_back, 0, 0);
	sheet_slide(sht_win, 10, 100);
	mx = (binfo->scrnx - 16) / 2; 
	my = (binfo->scrny - 28 - 16) / 2;
	sheet_slide(sht_mouse, mx, my);
	sheet_updown(sht_back,  0);				//将back图层设置到图层序列的最低
	sheet_updown(sht_win, 1);				//因为现在只有两个图层，将mouse放在第二个图层
	sheet_updown(sht_mouse, 2);	
	putfonts8_asc(buf_back, binfo->scrnx, 180, 100, COL8_FFFFFF, "Thanos OS");
	putfonts8_asc(buf_back, binfo->scrnx, 180, 120, COL8_FFFFFF, "Auther Thanos");
	putfonts8_asc(buf_back, binfo->scrnx, 180, 140, COL8_FFFFFF, "Write in 2021.9.4");
	sprintf(s, "memory : %d MB, free  : %d KB", memtotal/(1024*1024), memman_total(memman)/1024);
	putfonts8_asc(buf_back, binfo->scrnx, 0, 32, COL8_FFFFFF, s);
	sprintf(s, "(%d, %d)", mx, my);
	putfonts8_asc(buf_back, binfo->scrnx, 20, 70, COL8_FFFFFF, s);	

	sheet_refresh(sht_back, 0, 0, binfo->scrnx, binfo->scrny);
	
	for (;;) {
		counter++;
		sprintf(s, "%010d", counter);
		boxfill8(buf_win, 160,COL8_C6C6C6,40, 28, 119, 43);
		putfonts8_asc(buf_win, 160,40,28,COL8_000000, s);

		sheet_refresh(sht_win, 40, 28, 120, 44);
		io_cli();
		if( fifo8_status(&keyfifo) + fifo8_status(&mousefifo) == 0 ){
			//io_stihlt();		//先要关中断再进hlt
			io_sti();
		}else{
			if (fifo8_status(&keyfifo) != 0) {
				i = fifo8_get(&keyfifo);
				io_sti();
				sprintf(s, "%02X", i);
				boxfill8(buf_back, binfo->scrnx, COL8_008484,  0, 16, 15, 31);
				putfonts8_asc(buf_back, binfo->scrnx, 0, 16, COL8_FFFFFF, s);
				sheet_refresh(sht_back, 0, 16, 16, 32);
			} else if (fifo8_status(&mousefifo) != 0) {
				i = fifo8_get(&mousefifo);
				io_sti();
				if( mouse_decode(&mdec, i) != 0){
					sprintf(s, "[lcr %4d %4d]", mdec.x, mdec.y);
					//sprintf(s, "%02X %02X %02X", mdec->buf[0], mdec->buf[1], mdec->buf[2]);
					//boxfill8(binfo->vram, binfo->scrnx, COL8_008484, 32, 16, 47, 31);   会出现问题
					if((mdec.btn & 0x01) != 0){
						s[1] = 'L';
					}
					if((mdec.btn & 0x02) != 0){
						s[3] = 'R';
					}
					if((mdec.btn & 0x04) != 0){
						s[2] = 'C';
					}
					boxfill8(buf_back, binfo->scrnx, COL8_008484, 32, 16, 32 + 15 * 8 - 1, 31);
					putfonts8_asc(buf_back, binfo->scrnx, 32, 16, COL8_FFFFFF, s);
					sheet_refresh(sht_back, 32, 16, 32 + 15 * 8, 32);
					//移动鼠标指针
					//boxfill8(buf_back, binfo->scrnx, COL8_008484, mx, my, mx + 15, my + 15);	//隐藏鼠标
					//鼠标的位置增量
					mx += mdec.x;
					my += mdec.y;
					//不能让鼠标跑到屏幕的外面
					if(mx < 0){
						mx = 0;
					}
					if(my < 0){
						my = 0;
					}
					if(mx > binfo->scrnx-16){
						mx = binfo->scrnx-16;
					}
					if(my > binfo->scrny-16){
						my = binfo->scrny-16;
					}
					sprintf(s, "(%3d %3d)", mx, my);
					boxfill8(buf_back, binfo->scrnx, COL8_008484,  0, 0, 79, 15);    //隐藏坐标
					putfonts8_asc(buf_back, binfo->scrnx, 0, 0, COL8_FFFFFF, s);	//显示坐标
					//putblock8_8(buf_back, binfo->scrnx, 16, 16, mx, my, mcursor, 16);	//在系统中填充鼠标的颜色，描画坐标
					sheet_refresh(sht_back, 0, 0, 80, 16);
					sheet_slide(sht_mouse, mx, my);
					
				}
			}
		}
	}
}



