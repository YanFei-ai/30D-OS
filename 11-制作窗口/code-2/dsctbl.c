#include "bootpack.h"

void init_gdtidt(void)
{
	struct SEGMENT_DESCRIPTOR *gdt = (struct SEGMENT_DESCRIPTOR *) ADR_GDT;  //gdt的起始地址，内存中的空闲位置，0x270000-0x27ffff,8192个段，8192*8=64K
	struct GATE_DESCRIPTOR    *idt = (struct GATE_DESCRIPTOR    *) ADR_IDT;	//idt的起始地址，内存中的空闲位置, 0x26f800-0X26FFFF,256个entry 256*8=2K
	int i;
	for (i = 0; i <= LIMIT_GDT / 8; i++) {
		set_segmdesc(gdt + i, 0, 0, 0);
	}
	set_segmdesc(gdt + 1, 0xffffffff,   0x00000000, AR_DATA32_RW);	//段号为1的段， 起始0x00000000,0xffffffff(4GB),表示CPU所能管理的全部内存本身，段的属性设置成0x4092
	//说明，因为bootpack.h已经在0x280000-0x2fffff了，所以对段号为2的段做这样的设置，就会执行bootpack.hrb
	set_segmdesc(gdt + 2, LIMIT_BOTPAK, ADR_BOTPAK, AR_CODE32_ER);	//段号为1的段， 起始0x00280000,0x0007ffff(512K),为bootpack.hrb准备的，用这个段可以执行bootpack.hrb，段的属性设置成0x409a
	
	load_gdtr(LIMIT_GDT, ADR_GDT);	//0x0027000000开始，加载64KB的GDT

	for (i = 0; i <= LIMIT_IDT / 8; i++) {
		set_gatedesc(idt + i, 0, 0, 0);
	}
	load_idtr(LIMIT_IDT, ADR_IDT);	//加载0x26f800-0X26FFFF,256个entry 256*8=2K

    set_gatedesc(idt + 0x21, (int) asm_inthandler21, 2 * 8, AR_INTGATE32);
	set_gatedesc(idt + 0x27, (int) asm_inthandler27, 2 * 8, AR_INTGATE32);
	set_gatedesc(idt + 0x2c, (int) asm_inthandler2c, 2 * 8, AR_INTGATE32);

	return;
} 

/*初始化GDT的函数
*base	段的起始地址
*limit	段的大小
*ar		段的一些访问权限
*/
void set_segmdesc(struct SEGMENT_DESCRIPTOR *sd, unsigned int limit, int base, int ar)
{
	//对段的属性进行设置
	if (limit > 0xfffff) {
		//段的limit>0xffff之后，就要做一些设置
		ar |= 0x8000; /* G_bit = 1 G_bit = 1.说明是1G以上的段空间*/
		limit /= 0x1000;
	}
	sd->limit_low    = limit & 0xffff;
	sd->base_low     = base & 0xffff;
	sd->base_mid     = (base >> 16) & 0xff;
	sd->access_right = ar & 0xff;
	sd->limit_high   = ((limit >> 16) & 0x0f) | ((ar >> 8) & 0xf0);
	sd->base_high    = (base >> 24) & 0xff;
	return;
}

/*初始化IDT的函数
*base	段的起始地址
*offset	段的大小
*ar		段的一些访问权限
*/
void set_gatedesc(struct GATE_DESCRIPTOR *gd, int offset, int selector, int ar)
{
	gd->offset_low   = offset & 0xffff;
	gd->selector     = selector;
	gd->dw_count     = (ar >> 8) & 0xff;   // 高8位
	gd->access_right = ar & 0xff;
	gd->offset_high  = (offset >> 16) & 0xffff;
	return;
}

