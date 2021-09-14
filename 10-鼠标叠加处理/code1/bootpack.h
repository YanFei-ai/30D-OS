#include <stdio.h>

/* asmhead.nas */
struct BOOTINFO { /* 0x0ff0-0x0fff */
	char cyls; /* �u�[�g�Z�N�^�͂ǂ��܂Ńf�B�X�N��ǂ񂾂̂� */
	char leds; /* �u�[�g���̃L�[�{�[�h��LED�̏�� */
	char vmode; /* �r�f�I���[�h  ���r�b�g�J���[�� */
	char reserve;
	short scrnx, scrny; /* ��ʉ𑜓x */
	char *vram;
};
#define ADR_BOOTINFO	0x00000ff0

/* naskfunc.nas */
void io_hlt(void);
void io_cli(void);
void io_sti(void);
void io_stihlt(void);
void io_out8(int port, int data);
int  io_in8(int port);
int io_load_eflags(void);
void io_store_eflags(int eflags);
void load_gdtr(int limit, int addr);
void load_idtr(int limit, int addr);
void asm_inthandler21(void);
void asm_inthandler27(void);
void asm_inthandler2c(void);
int  load_cr0(void);
void store_cr0(int cr0);

/* graphic.c */
void init_palette(void);//设置调色板
void set_palette(int start, int end, unsigned char *rgb);
void boxfill8(unsigned char *vram, int xsize, unsigned char c, int x0, int y0, int x1, int y1);//画出矩形
void init_screen8(char *vram, int x, int y); //填充屏幕背景
void putfont8(char *vram, int xsize, int x, int y, char c, char *font);  //打印字符
void putfonts8_asc(char *vram, int xsize, int x, int y, char c, unsigned char *s); //打印字符串
void init_mouse_cursor8(char *mouse, char bc); //设置鼠标的大小和颜色
void putblock8_8(char *vram, int vxsize, int pxsize,int pysize, int px0, int py0, char *buf, int bxsize); //将init_mouse函数中设置的鼠标颜色填充到vram中

#define COL8_000000		0
#define COL8_FF0000		1
#define COL8_00FF00		2
#define COL8_FFFF00		3
#define COL8_0000FF		4
#define COL8_FF00FF		5
#define COL8_00FFFF		6
#define COL8_FFFFFF		7
#define COL8_C6C6C6		8
#define COL8_840000		9
#define COL8_008400		10
#define COL8_848400		11
#define COL8_000084		12
#define COL8_840084		13
#define COL8_008484		14
#define COL8_848484		15

/*dsctbl.c*/
//在一个段描述的entry中表示一个段需要的信息
//	段的大小
//	段的起始地址
//	短的access属性(禁止写入、禁止执行、系统专用等)
struct SEGMENT_DESCRIPTOR {
	short limit_low, base_low;
	char base_mid, access_right;
	char limit_high, base_high;
};
struct GATE_DESCRIPTOR {
	short offset_low, selector;
	char dw_count, access_right;
	short offset_high;
};
void init_gdtidt(void);
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar);
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar);
#define ADR_IDT			0x0026f800
#define LIMIT_IDT		0x000007ff
#define ADR_GDT			0x00270000
#define LIMIT_GDT		0x0000ffff
#define ADR_BOTPAK		0x00280000
#define LIMIT_BOTPAK	0x0007ffff
#define AR_DATA32_RW	0x4092
#define AR_CODE32_ER	0x409a
#define AR_INTGATE32	0x008e

/* int.c */
void init_pic(void);
void inthandler21(int *esp);
void inthandler27(int *esp);
void inthandler2c(int *esp);
#define PIC0_ICW1		0x0020
#define PIC0_OCW2		0x0020
#define PIC0_IMR		0x0021
#define PIC0_ICW2		0x0021
#define PIC0_ICW3		0x0021
#define PIC0_ICW4		0x0021
#define PIC1_ICW1		0x00a0
#define PIC1_OCW2		0x00a0
#define PIC1_IMR		0x00a1
#define PIC1_ICW2		0x00a1
#define PIC1_ICW3		0x00a1
#define PIC1_ICW4		0x00a1
#define PORT_KEYDAT		0x0060



/*fifo.c*/
struct FIFO8 {
	unsigned char *buf;		//接受键盘的数据
	int p, q, size, free, flags;
	/*p 下一个数据写入的地址（next_w）；
	 *q 下一个数据读出的地址（next_r）
	 *size 缓冲区大小
	 *free 缓冲区的剩余大小
	 *flags 有无数据的标志位
	*/					
} ;

void fifo8_init(struct FIFO8 *fifo, int size, unsigned char *buf);
//存入
int fifo8_put(struct FIFO8 *fifo, unsigned char data);
//读出
int fifo8_get(struct FIFO8 *fifo);

int fifo8_status(struct FIFO8 *fifo);


/*keyboard.c*/
#define PORT_KEYDAT				0x0060
#define PORT_KEYSTA				0x0064
#define PORT_KEYCMD				0x0064
#define KEYSTA_SEND_NOTREADY	0x02
#define KEYCMD_WRITE_MODE		0x60
#define KBC_MODE				0x47

void init_keyboard(void);
void wait_KBC_sendready(void);
void inthandler21(int *esp);

/*mouse.c*/
struct MOUSE_DEC {
	unsigned char buf[3], phase;
	int x, y, btn;
};

#define KEYCMD_SENDTO_MOUSE		0xd4
#define MOUSECMD_ENABLE			0xf4
void inthandler2c(int *esp);
void enable_mouse(void);	//激活鼠标
int mouse_decode(struct MOUSE_DEC *mdec, unsigned char dat);	//接受鼠标数据的函数，对传入的结构体进行赋值,并让鼠标移动起来


/*内存检查 memory.c*/
#define EFLAGS_AC_BIT		0x00040000
#define CR0_CACHE_DISABLE	0x60000000
#define MEMMAN_FREES		4096		//大约32KB
#define MEMMAN_ADDR			0x003c0000  //memman需要3KB的地址，从ox003c0000开始

struct FREEFIFO {	//可用信息
	unsigned int addr, size;
};

struct MEMMAN {		//内存管理
	int frees, maxfrees, lostsize, losts;
	struct FREEFIFO free[MEMMAN_FREES];
};

unsigned int memtest(unsigned int start, unsigned int end);
unsigned int memtest_sub(unsigned int start, unsigned int end);

void memman_init(struct MEMMAN *man);
unsigned int memman_total(struct MEMMAN *man);
unsigned int memman_alloc(struct MEMMAN *man, unsigned int size);			//在MEMMAN结构体中，要申请多大的内存size，字节为单位
int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size);	//释放内存，以字节为单位
unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size);		//申请内存，以0x1000字节(4k)为单位
int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size); //以4K字节向上释放内存 ，向上舍入roundup

/*sheet.c*/
#define MAX_SHEETS	256			//假设最大的图层数是256
#define SHEET_USE 1

struct SHEET {
	unsigned char *buf;		//address,图层上所描画内容的地址
	int bxsize, bysize;		//bxsize*bysize 图层大小；
	int vx0, vy0;			//vx0*vy0 图层在湖面上的位置坐标
	int col_inv;			//色号
	int height;				//height表示该图层在第几层
	int flags;				//存放图层的设定值，是否使用等
};

struct SHTCTL{
	unsigned char *vram;
	int xsize, ysize;		//vram.xsize,ysize代表着VRAM的地址和画面大小，来自BOOTINFO，但是每次取比较麻烦，预先将他们复制到结构体的成员变量中
	int top;				//代表最上层的高度
	struct SHEET *sheets[MAX_SHEETS];	//记忆地址变量，就是对sheet0中存放的图层进行升序排序，并且保存 256*8字节
	struct SHEET sheet0[MAX_SHEETS];	//存放所有的图层信息 256*32字节
};
struct SHTCTL *shtctl_init(struct MEMMAN *man, unsigned char *vram, int xsize, int ysize);	//初始化SHTCTL,并为它申请空间
struct SHEET* sheet_alloc(struct SHTCTL *ctl);      //从init返回的ctl中申请还未被使用的图层
void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv);     //设置图层的缓冲区大小和透明色
void sheet_updown(struct SHTCTL *ctl, struct SHEET *sht, int height);      //决定这个图层在图层管理序列中的位置
void sheet_refresh(struct SHTCTL *ctl);    //刷新图层的排列
void sheet_slide(struct SHTCTL *ctl, struct SHEET *sht, int vx0, int vy0);	//平移，不会改变图层的高度

