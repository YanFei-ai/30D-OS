#include "bootpack.h"

struct SHTCTL *shtctl_init(struct MEMMAN *man, unsigned char *vram, int xsize, int ysize)	//初始化SHTCTL,并为它申请空间
{
	struct SHTCTL *ctl;
	int i;
	ctl = (struct SHTCTL *)memman_alloc_4k(man, sizeof(struct SHTCTL));
    if (ctl == 0){
        goto err;
    }
    ctl->vram = vram;
    ctl->xsize = xsize;
    ctl->ysize = ysize;
    ctl->top = -1;      //此时一个SHEET都没有
    for(i=0; i<MAX_SHEETS; i++){
        ctl->sheet0[i].flags = 0;       //标记为未使用
    }
err:
    return ctl;
}

struct SHEET* sheet_alloc(struct SHTCTL *ctl)       //从init返回的ctl中申请还未被使用的图层
{
    struct SHEET *sht;
    int i;
    for(i=0; i<MAX_SHEETS; i++){
        if(ctl->sheet0[i].flags == 0){
            sht = &(ctl->sheet0[i]);
            sht->flags = SHEET_USE; //标记已使用
            sht->height = -1;       //隐藏，表示该图层还没有加入到管理序列 ctl->sheets中
            return sht;
        }
    }
    return 0;       //没有可使用的图层
}

void sheet_setbuf(struct SHEET *sht, unsigned char *buf, int xsize, int ysize, int col_inv)     //设置图层的缓冲区大小和透明色
{
    sht->buf = buf;
	sht->bxsize = xsize;
	sht->bysize = ysize;
	sht->col_inv = col_inv;
	return;
} 

void sheet_updown(struct SHTCTL *ctl, struct SHEET *sht, int height)      //决定这个图层在图层管理序列中的位置
{
    int h, old=sht->height; //保存原来这个图层在ctl中的位置（高度）
    /*同时，存在一些出错的可能，要做出修正*/
    if(height > ctl->top+1){
        height = ctl->top+1;
    }
    if(height < -1){
        height = -1;
    }
    sht->height = height;       //先在图层结构体中，设定高度
    //按照高度，排除该图层在管理序列中的位置（高度），即对sheets进行重新排序
    if(old > height){       //比以前的低
        if(height >= 0){        //往上提
            for(h=old; h>height; h--){
                ctl->sheets[h] = ctl->sheets[h-1];
                ctl->sheets[h]->height = h;
            }
            ctl->sheets[height] = sht;
        }else{      //隐藏
            if(ctl->top > old){ //该图层原来的位置在图层序列中
                for(h=old; h>height; h++){
                    for(h=old; h<ctl->top; h++){       //old后面的下走
                        ctl->sheets[h] = ctl->sheets[h+1];
                        ctl->sheets[h]->height = h;
                    }
                }
            }
            ctl->top--;     //图层中减少一个
        }
        sheet_refresh(ctl);
    }
    else if(old < height){    //比以前的高
        if(old >= 0 ){  //把height之前的往下拉
            for(h=old; h<height; h++){
                ctl->sheets[h] = ctl->sheets[h+1];
                ctl->sheets[h] = h;
            }
            ctl->sheets[height] = sht;
        }else{      //由隐藏状态转为显示状态
            //把height之后的都向上拉，并且top要加一
            for(h=ctl->top; h>height;h--){
                ctl->sheets[h+1] = ctl->sheets[h];
                ctl->sheets[h+1] = h+1;
            }
            ctl->sheets[height] = sht;
            ctl->top++;
        }
        sheet_refresh(ctl);
    }
    return;
}

void sheet_refresh(struct SHTCTL *ctl){     //刷新图层的排列
    int h, bx, by, vx, vy;
    unsigned char *buf, c, *vram = ctl->vram;
    struct SHEET *sht;
    for(h=0; h<=ctl->top; h++){
        sht = ctl->sheets[h];   //从最下面的图层开始，往上刷新
        buf = sht->buf;
        for(by=0; by<sht->bysize;by++){     //y方向，x方向依次刷新
            vy = sht->vy0+by;
            for(bx=0; bx<sht->bxsize;bx++){
                vx = sht->vx0+bx;
                c = buf[by*sht->bxsize+bx];
                if(c != sht->col_inv){
                    vram[vy * ctl->xsize +vx] = c;
                }
            }
        }
    }
    return;
}

//平移，不会改变图层的高度
void sheet_slide(struct SHTCTL *ctl, struct SHEET *sht, int vx0, int vy0)
{
	sht->vx0 = vx0;
	sht->vy0 = vy0;
	if (sht->height >= 0) { //正在显示的图层
		sheet_refresh(ctl); //按新图层的西南西刷新画面
	}
	return;
}


//释放已使用图层的内存
void sheet_free(struct SHTCTL *ctl, struct SHEET *sht)
{
	if (sht->height >= 0) {
		sheet_updown(ctl, sht, -1); //如果正在显示，设置为隐藏
	}
	sht->flags = 0; //将图层设s置为未使用
	return;
}


