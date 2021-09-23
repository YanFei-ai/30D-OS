OBJS_BOOTPACK = bootpack.obj naskfunc.obj hankaku.obj graphic.obj dsctbl.obj int.obj fifo.obj keyboard.obj mouse.obj memory.obj sheet.obj

TOOLPATM	= ../z_tools/
INCPATH		= $(TOOLPATM)haribote/

MAKE		= $(TOOLPATM)make.exe -r
NASK		= $(TOOLPATH)nask.exe
CC1			= $(TOOLPATM)cc1.exe -I$(INCPATH) -Os -Wall -quiet
GAS2NASK = $(TOOLPATH)gas2nask.exe -a
OBJ2BIM  = $(TOOLPATH)obj2bim.exe
MAKEFONT = $(TOOLPATH)makefont.exe
BIN2OBJ  = $(TOOLPATH)bin2obj.exe
BIM2HRB  = $(TOOLPATH)bim2hrb.exe
RULEFILE = $(INCPATH)haribote.rul
EDIMG		= $(TOOLPATH)edimg.exe
IMGTOL		= $(TOOLPATH)imgtol.com
#C:/Users/18817/Desktop/learn-file/1-Thanos-OS/30-OS/tolset
IMGIN_FILE	= ../z_tools/fdimg0at.tek
QEMU_TARGET	= "../z_tools/qemu/fdimage0.bin"
QEMU		= ../z_tools/qemu
COPY		= copy
DEL			= del

OS_TARGET	= Thanosos.img

default:
	$(MAKE) img

ipl10.bin:ipl10.nas Makefile
	$(NASK) ipl10.nas ipl10.bin ipl.lst

asmhead.bin:asmhead.nas Makefile
	$(NASK) asmhead.nas asmhead.bin asmhead.lst

#bootpack.gas : bootpack.c Makefile
#	$(CC1) -o bootpack.gas bootpack.c

#bootpack.nas : bootpack.gas Makefile
#	$(GAS2NASK) bootpack.gas bootpack.nas

#bootpack.obj : bootpack.nas Makefile
#	$(NASK) bootpack.nas bootpack.obj bootpack.lst

#naskfunc.obj : naskfunc.nas Makefile
#	$(NASK) naskfunc.nas naskfunc.obj naskfunc.lst

hankaku.bin : hankaku.txt Makefile
	$(MAKEFONT) hankaku.txt hankaku.bin

hankaku.obj : hankaku.bin Makefile
	$(BIN2OBJ) hankaku.bin hankaku.obj _hankaku

bootpack.bim :  $(OBJS_BOOTPACK) Makefile
	$(OBJ2BIM) @$(RULEFILE) out:bootpack.bim stack:3136k map:bootpack.map $(OBJS_BOOTPACK)
# 3MB+64KB=3136KB

bootpack.hrb : bootpack.bim Makefile
	$(BIM2HRB) bootpack.bim bootpack.hrb 0

haribote.sys : asmhead.bin bootpack.hrb Makefile
	copy /B asmhead.bin+bootpack.hrb haribote.sys

$(OS_TARGET):ipl10.bin haribote.sys Makefile
	$(EDIMG) imgin:$(IMGIN_FILE) \
		wbinimg src:ipl10.bin len:512 from:0 to:0 \
		copy from:haribote.sys to:@: \
		imgout:$(OS_TARGET)

#%.gas : %.c Makefile
#	$(CC1) -o $*.gas $*.c
#$<依赖文件   $@结果文件
#.c.gas:
#	$(CC1) -o $@ $<

#.gas.nas:
#	$(GAS2NASK) $< $@


%.gas : %.c Makefile
	$(CC1) -o $*.gas $*.c

%.nas : %.gas Makefile
	$(GAS2NASK) $*.gas $*.nas

%.obj : %.nas Makefile
	$(NASK) $*.nas $*.obj $*.lst


#命令，make+命令：生成相应的结果
img:
	$(MAKE)  $(OS_TARGET)

#run :
#	$(MAKE) img
#	copy Thanosos.img ..\z_tools\qemu\fdimage0.bin
#	../z_tools/make.exe -C ../z_tools/qemu

run :
	$(MAKE) img
	$(COPY) $(OS_TARGET) $(QEMU_TARGET)
	$(MAKE) -C $(QEMU)

#64位系统不能install，需要32位系统
#install :
#	../z_tools/make.exe img
#	../z_tools/imgtol.com w a: helloos.img

clean :
	-$(DEL) *.bin
	-$(DEL) *.lst
#	-$(DEL) *.gas
	-$(DEL) *.obj
#	-$(DEL) bootpack.nas
#	-$(DEL) bootpack.map
	-$(DEL) bootpack.bim
	-$(DEL) bootpack.hrb
	-$(DEL) haribote.sys

all_clean :
	$(MAKE) clean
	$(DEL) $(OS_TARGET)