#include "bootpack.h"

//设置调色板
void init_palette(void)
{
	static unsigned char table_rgb[16*3] = {
		0x00, 0x00, 0x00,	/*0:黑 */
		0xff, 0x00, 0x00,	/*1:亮红*/
		0x00, 0xff, 0x00,	/*2:亮绿*/
		0xff, 0xff, 0x00,	/*3:亮黄*/
		0x00, 0x00, 0xff,	/*4:亮蓝*/
		0xff, 0x00, 0xff,	/*5:亮紫*/
		0x00, 0xff, 0xff,	/*6:浅亮蓝*/
		0xff, 0xff, 0xff,	/*7:白*/
		0xc6, 0xc6, 0xc6,	/*8:亮灰*/
		0x84, 0x00, 0x00,	/*9:暗红*/
		0x00, 0x84, 0x00,	/*10:暗绿*/
		0x84, 0x84, 0x00,	/*11:暗黄*/
		0x00, 0x00, 0x84,	/*12:暗青*/
		0x84, 0x00, 0x84,	/*13:暗紫*/
		0x00, 0x84, 0x84,	/*14:浅暗蓝*/
		0x84, 0x84, 0x84	/*15:暗灰*/

	};
	set_palette(0, 15, table_rgb);
	return;
}

void set_palette(int start, int end, unsigned char *rgb)
{
	int i;
	int eflags;
	eflags = io_load_eflags();	//记录eflags的值，即记录中断许可标志的值
	io_cli();					//禁止中断，将中断许可标志置为0
	io_out8(0x03c8, start);
	for(i = start; i <=end; i++){
		io_out8(0x03c9, rgb[0]/4);
		io_out8(0x03c9, rgb[1]/4);
		io_out8(0x03c9, rgb[2]/4);
		rgb += 3;
	}
	io_store_eflags(eflags);	//复原中断许可标志
	return;
}

//绘制矩形
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1)
{
	int x, y;
	for (y = y0; y <= y1; y++) {
		for (x = x0; x <= x1; x++)
			vram[y * xsize + x] = c;  //像素点在vram数组中是按顺序存放的
	}
	return;
}

//绘制系统的背景界面
void init_screen8(char *vram, int x, int y)
{
	boxfill8(vram, x, COL8_008484,  0,     0,      x -  1, y - 29);
	boxfill8(vram, x, COL8_C6C6C6,  0,     y - 28, x -  1, y - 28);
	boxfill8(vram, x, COL8_FFFFFF,  0,     y - 27, x -  1, y - 27);
	boxfill8(vram, x, COL8_C6C6C6,  0,     y - 26, x -  1, y -  1);

	boxfill8(vram, x, COL8_FFFFFF,  3,     y - 24, 59,     y - 24);
	boxfill8(vram, x, COL8_FFFFFF,  2,     y - 24,  2,     y -  4);
	boxfill8(vram, x, COL8_848484,  3,     y -  4, 59,     y -  4);
	boxfill8(vram, x, COL8_848484, 59,     y - 23, 59,     y -  5);
	boxfill8(vram, x, COL8_000000,  2,     y -  3, 59,     y -  3);
	boxfill8(vram, x, COL8_000000, 60,     y - 24, 60,     y -  3);

	boxfill8(vram, x, COL8_848484, x - 47, y - 24, x -  4, y - 24);
	boxfill8(vram, x, COL8_848484, x - 47, y - 23, x - 47, y -  4);
	boxfill8(vram, x, COL8_FFFFFF, x - 47, y -  3, x -  4, y -  3);
	boxfill8(vram, x, COL8_FFFFFF, x -  3, y - 24, x -  3, y -  3);
	return;
}

//打印字符
//putfont8(binfo->vram, binfo->scrnx, 10, 10, COL8_FFFFFF, hankaku + 'T'*16);   打印字符T
void putfont8(char *vram, int xsize, int x, int y, char c, char *font)
{
	int i;
	char *p, d /* data */;
	for (i = 0; i < 16; i++) {
		p = vram + (y + i) * xsize + x;
		d = font[i];
		if ((d & 0x80) != 0) { p[0] = c; }  //vram[(y+i)*xscreen+x]
		if ((d & 0x40) != 0) { p[1] = c; }
		if ((d & 0x20) != 0) { p[2] = c; }
		if ((d & 0x10) != 0) { p[3] = c; }
		if ((d & 0x08) != 0) { p[4] = c; }
		if ((d & 0x04) != 0) { p[5] = c; }
		if ((d & 0x02) != 0) { p[6] = c; }
		if ((d & 0x01) != 0) { p[7] = c; }
	}
	return;
}

//打印字符串
void putfonts8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s)
{
	extern char hankaku[4096];
	for (; *s != 0x00; s++) {
		putfont8(vram, xsize, x, y, c, hankaku + *s * 16);
		x += 8;
	}
	return;
}

//设置鼠标的大小，颜色 ,将颜色填充到mouse_1数组中，就是鼠标数组
void init_mouse_cursor8(char *mouse, char bc)
{
	//鼠标的结构
	int x, y;
	static char cursor[16][16] = {
		"**************..",
		"*OOOOOOOOOOO*...",
		"*OOOOOOOOOO*....",
		"*OOOOOOOOO*.....",
		"*OOOOOOOO*......",
		"*OOOOOOO*.......",
		"*OOOOOOO*.......",
		"*OOOOOOOO*......",
		"*OOOO**OOO*.....",
		"*OOO*..*OOO*....",
		"*OO*....*OOO*...",
		"*O*......*OOO*..",
		"**........*OOO*.",
		"*..........*OOO*",
		"............*OO*",
		".............***"
	};
	for (y = 0; y < 16; y++) {
		for (x = 0; x < 16; x++) {
			if (cursor[y][x] == '*') {
				mouse[y * 16 + x] = COL8_000000;
			}
			if (cursor[y][x] == 'O') {
				mouse[y * 16 + x] = COL8_FFFFFF;
			}
			if (cursor[y][x] == '.') {
				mouse[y * 16 + x] = bc;
			}
		}
	}
	return;
}

//在系统中填充鼠标的颜色
//mx0,my0 鼠标在系统中出现的地方 
//mxsize,mysize 鼠标指针的大小
//char *buf, int bsize  buf，前面填充鼠标指针颜色的数组
void putblock8_8(char *vram, int vxsize, int pxsize,
	int pysize, int px0, int py0, char *buf, int bxsize)
{
	int x, y;
	for (y = 0; y < pysize; y++) {
		for (x = 0; x < pxsize; x++) {
			vram[(py0 + y) * vxsize + (px0 + x)] = buf[y * bxsize + x];
		}
	}
	return;
}



//显示窗口
void make_windows(unsigned char *buf, int xsize, int ysize, char *title)
{
	//鼠标的结构
	int x, y;


static char cursor[14][16] = {
		"OOOOOOOOOOOOOOO@",
		"OQQQQQQQQQQQQQ$@",
		"OQQQQQQQQQQQQQ$@",
		"OQQQ@@QQQQ@@QQ$@",
		"OQQQQ@@QQ@@QQQ$@",
		"OQQQQQ@@@@QQQQ$@",
		"OQQQQQQ@@QQQQQ$@",
		"OQQQQQ@@@@QQQQ$@",
		"OQQQQ@@QQ@@QQQ$@",
		"OQQQ@@QQQQ@@QQ$@",
		"OQQQQQQQQQQQQQ$@",
		"OQQQQQQQQQQQQQ$@",
		"O$$$$$$$$$$$$$$@",
		"@@@@@@@@@@@@@@@@"
	};

	//画出窗口
	boxfill8(buf, xsize, COL8_C6C6C6, 0,         0,         xsize - 1, 0        );
	boxfill8(buf, xsize, COL8_FFFFFF, 1,         1,         xsize - 2, 1        );
	boxfill8(buf, xsize, COL8_C6C6C6, 0,         0,         0,         ysize - 1);
	boxfill8(buf, xsize, COL8_FFFFFF, 1,         1,         1,         ysize - 2);
	boxfill8(buf, xsize, COL8_848484, xsize - 2, 1,         xsize - 2, ysize - 2);
	boxfill8(buf, xsize, COL8_000000, xsize - 1, 0,         xsize - 1, ysize - 1);
	boxfill8(buf, xsize, COL8_C6C6C6, 2,         2,         xsize - 3, ysize - 3);
	boxfill8(buf, xsize, COL8_000084, 3,         3,         xsize - 4, 20       );
	boxfill8(buf, xsize, COL8_848484, 1,         ysize - 2, xsize - 2, ysize - 2);
	boxfill8(buf, xsize, COL8_000000, 0,         ysize - 1, xsize - 1, ysize - 1);
	//打印窗口名称
	putfonts8_asc(buf, xsize, 24, 4, COL8_FFFFFF, title);

	for (y = 0; y < 14; y++) {
		for (x = 0; x < 16; x++) {
			if (cursor[y][x] == '@') {
				buf[(5 + y) * xsize + (xsize - 21 + x)] = COL8_000000;
			}
			if (cursor[y][x] == 'O') {
				buf[(5 + y) * xsize + (xsize - 21 + x)] =  COL8_C6C6C6;
			}
			if (cursor[y][x] == 'Q') {
				buf[(5 + y) * xsize + (xsize - 21 + x)] = COL8_FFFFFF;
			}	

			if (cursor[y][x] == '$') {
				buf[(5 + y) * xsize + (xsize - 21 + x)] = COL8_C6C6C6;
			}
			//win[y * 16 + x]

			//if (cursor[y][x] == '.') {
			//	win[y * 16 + x] = bc;    [(5 + y) * xsize + (xsize - 21 + x)] 
			//}
			
		}
	}
	return;
}

