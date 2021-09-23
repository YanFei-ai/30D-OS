#include "bootpack.h"



/*关闭缓存--检查内存--开启缓存*/
unsigned int memtest(unsigned int start, unsigned int end)
{
	char flg486 = 0;
	unsigned int eflag, cr0, i;

	eflag = io_load_eflags();		//先保存系统原本的eflags
	eflag |= EFLAGS_AC_BIT;			//将eflag的bit18置为1
	io_store_eflags(eflag);			

	eflag = io_load_eflags();		//获取改变了之后的eflags
	//比较，如果是386，bit18会变回1
	if((eflag & EFLAGS_AC_BIT) != 0){
		flg486 = 1;
	}

	eflag &= ~EFLAGS_AC_BIT;		//将eflag的bit18置为0
	io_store_eflags(eflag);			//此时，系统的eflag的bit18已经置为0

	if( flg486 != 0 ){
		//486才需要关闭cache
		cr0 = load_cr0();
		cr0 |= CR0_CACHE_DISABLE;		//关闭缓存
		store_cr0(cr0);
	}

	i = memtest_sub(start, end);	//检查内存函数

	if( flg486 != 0 ){
		//486才需要关闭cache
		cr0 = load_cr0();
		cr0 &= ~CR0_CACHE_DISABLE;		//关闭缓存
		store_cr0(cr0);
	}
	return i;
}

/*内存测试函数, 因为编译器优化的问题，改用汇编程序实现
//失败，返回终止地址值，全部通过，返回end值
unsigned int memtest_sub(unsigned int start, unsigned int end)
{
	unsigned int *p, i, old;
	unsigned pat0 = 0xaa55aa55;
	unsigned pat1 = 0x55aa55aa;

	for(i=start; i<=end; i+=0x1000){
		*p = (unsigned int *)(i + 0xffc);	//一次检查4KB,即0x1000
		old = *p;			//记住内存中先前的值
		*p = pat0;			//试写
		*p ^= 0xffffffff;	//反转
		if( *p != pat1){	//检查是不是反转成功
not_memory:
			*p = old;		//反转失败，写回原来的值，跳出循环，函数返回地址
			break;
		}
		*p ^= 0xffffffff;	//第一次反转成功过后，再次反转
		if( *p != pat0){	//检查是不是反转成功
			goto not_memory;//失败，跳转到失败处理的地方
		}
		*p = old;			//检查成功，写回原来的值
	}
	return i;
}
*/



void memman_init(struct MEMMAN *man)
{
	/*初始化可用信息列表*/
	man->frees    = 0;		//可用信息数目
	man->maxfrees  = 0;		//用于观察可用信息，frees的最大值,在free增加时，它也要增加
	man->lostsize = 0;		//适当失败的内存大小的总和
	man->losts	 = 0;		//释放失败的次数
	//return ;
}

unsigned int memman_total(struct MEMMAN *man)
{
	/*计算total 空余内存*/
	unsigned int i, t=0;
	for(i = 0; i < man->frees; i++){
		t += man->free[i].size;				//每一个列表的空余size都加入
	}
	return t;
}

unsigned int memman_alloc(struct MEMMAN *man, unsigned int size)			//在MEMMAN结构体中，要申请多大的内存ize
{
	unsigned int i, a;

	for(i = 0; i < man->frees; i++){	//遍历整个内存列表，寻找足够大的内存块
		if(size <= man->free[i].size){	//找到了足够大的内存
			a = man->free[i].addr;
			/*对分配完内存的这一条可用信息做处理*/
			//1. 可用信息表，该list，剩余内存的处理，将地址向后移动size，大小减去size
			man->free[i].addr += size;
			man->free[i].size -= size;
			//2. 处理完之后，进行判断：这条list，是否还有足够的内存，有就留下，没有就从可用信息表中删除，同时可用信息表要进行处理，填补空缺
			if(man->free[i].size == 0){
				man->frees--;
				//剩余可用信息表的处理
				for(; i < man->frees; i++){
					man->free[i] = man->free[i+1];
				}
			}
			return a;
		}
	}
	return 0;		//没有可用空间
}

int memman_free(struct MEMMAN *man, unsigned int addr, unsigned int size)		//向MEMMAN结构体中还多大的内存，以及内存的起始地址
{
	int i, j;

	/*1. 确定返还内存在MEMMAN中的位置*/
	for(i = 0; i < man->frees; i++){
		if(man->free[i].addr > addr){		//在内存中找到了addr所在的位置，在man->free[i].addr之前
			break;
		}
	}
	/*man->free[i-1].addr < addr < man->free[i].addr*/
	/* 插入内存时的情况
	 * 1. 找到位置之后，既可以和前一个list连接上，也可以和后一个list连接上
	 * 2. 不能与前一个list连接上
	 * 3. 前后都不能连接上，frees+1
	 * 4. free失败
	*/
	if(i > 0){		//前面有可用内存的情况下
		if(man->free[i-1].addr + man->free[i-1].size == addr){	//可以和前面可用空间的连接
			man->free[i-1].size += size;						//把要释放的内存加入到i+1的内存空间中
			if(i < man->frees){
				if(addr + size == man->free[i].addr){				//可以和后面连接上
					man->free[i-1].size += man->free[i].size;		//将i的而可用内存空间，也接续到i-1的后面，并且要做移位处理，i删除，后面前移
					man->frees--;
					/*位移处理*/
					for(; i < man->frees; i++){
						man->free[i] = man->free[i+1];				//结构体赋值
					}
				}
			}
			return 0;		//成功
		}
	}

	if (i < man->frees) {		//可以和后面链接，但是不能和前面连接
		if(addr + size == man->free[i].addr ){
			man->free[i].addr = addr;
			man->free[i].size += size;
			return 0;
		}
	}
	if (man->frees < MEMMAN_FREES) {
		//前面、后面都不能连接的，把原本i之后的list向后移动，将它插入到原本的i的位置(顺序不能变)
		for(j = man->frees; j > i; j--){
			man->free[j] = man->free[j-1];
		}
		man->frees++;
		/*同时内存空间的可用list的更新最大值*/
		if(man->maxfrees < man->frees){
			man->maxfrees = man->frees;
		}
		man->free[i].addr = addr;
		man->free[i].size = size;
		return 0;
	}
	/*不能移动，更新失败，lost+1，lostsize增加*/
	man->losts++;
	man->lostsize += size;
	return -1;		//失败
}

unsigned int memman_alloc_4k(struct MEMMAN *man, unsigned int size)		//申请内存，以0x1000字节(4k)为单位，向上舍入roundup
{
	unsigned int a;
	size = (size + 0xfff) & 0xfffff000;
	a = memman_alloc(man, size);
	return a;
}
int memman_free_4k(struct MEMMAN *man, unsigned int addr, unsigned int size)		//以4K字节向上释放内存 ，向上舍入roundup
{
	int i;
	size = (size + 0xfff) & 0xfffff000;
	i = memman_free(man, addr, size);
	return i;
}

