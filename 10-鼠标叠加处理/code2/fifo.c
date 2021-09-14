#include "bootpack.h"
#define FLAGS_OVERRUN		0x0001

//初始化fifo结构体没设定初始值
void fifo8_init(struct FIFO8 *fifo, int size, unsigned char *buf)
{
    fifo->size = size;
    fifo->free = size;   //缓冲区的大小
    fifo->buf = buf;
    fifo->flags = 0;
    fifo->p = 0;        //下一个数据的写入地址
    fifo->q = 0;        //下一个数据的d读出地址

    return ;
}

//存入
int fifo8_put(struct FIFO8 *fifo, unsigned char data)
{
    if(fifo->free == 0){
        //没有空余的空间
        fifo->flags |= FLAGS_OVERRUN;
        return -1;
    }
    fifo->buf[fifo->p] = data;
    fifo->p++;
    if(fifo->p == fifo->size){
        fifo->p = 0;
    }
    fifo->free--;
    return 0;
}

//读出
int fifo8_get(struct FIFO8 *fifo)
{
    int data;
    if(fifo->free == fifo->size){
        //缓冲区为空
        return -1;
    }
    data = fifo->buf[fifo->q];
    fifo->q++;
        if(fifo->q == fifo->size){
        fifo->q = 0;
    }
    fifo->free++;
    return data;
}

int fifo8_status(struct FIFO8 *fifo)
/* 返回状态 */
{
	return fifo->size - fifo->free;
}
