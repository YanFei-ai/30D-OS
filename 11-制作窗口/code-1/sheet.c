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
        ctl->sheet0[i].ctl = ctl;
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

void sheet_updown(struct SHEET *sht, int height)      //决定这个图层在图层管理序列中的位置
{
    struct SHTCTL *ctl;
    ctl = sht->ctl;
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
             sheet_refresh(sht, sht->vx0, sht->vy0, sht->vx0+sht->bxsize, sht->vy0+sht->bysize);
        }     
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
        sheet_refresh(sht, sht->vx0, sht->vy0, sht->vx0+sht->bxsize, sht->vy0+sht->bysize);
    }
    return;
}

void sheet_refresh(struct SHEET *sht,  int bx0, int by0, int bx1, int by1){     //刷新全部图层的排列
    struct SHTCTL *ctl = sht->ctl;
    if(sht->height >= 0){       //正在显示
        sheet_refreshsub(ctl, sht->vx0+bx0, sht->vy0+by0, sht->vx0+bx1, sht->vy0+by1);
    }
    return;
}

//平移，不会改变图层的高度
void sheet_slide(struct SHEET *sht, int vx0, int vy0)
{
    struct SHTCTL *ctl = sht->ctl;
    int old_vx0 = sht->vx0, old_vy0 = sht->vy0;     //记录移动前的显示位置
	sht->vx0 = vx0;
	sht->vy0 = vy0;
	if (sht->height >= 0) { //正在显示的图层
        sheet_refreshsub(ctl, old_vx0, old_vy0, old_vx0+sht->bxsize, old_vy0+sht->bysize);  //将移动之前的位置刷新
		sheet_refreshsub(ctl, vx0, vy0, vx0+sht->bxsize, vy0+sht->bysize); //按新图层的西南西刷新画面
	}
	return;
}


//释放已使用图层的内存
void sheet_free(struct SHEET *sht)
{
   // struct SHTCTL *ctl = sht->ctl;
	if (sht->height >= 0) {
		sheet_updown(sht, -1); //如果正在显示，设置为隐藏
	}
	sht->flags = 0; //将图层设s置为未使用
	return;
}

void sheet_refreshsub(struct SHTCTL *ctl, int vx0, int vy0, int vx1, int vy1)
{     
    //刷新一个范围内图层的排列 vx0-vy1
    int h, bx, by, vx, vy, bx0, by0, bx1, by1;
	unsigned char *buf, c, *vram = ctl->vram;
	struct SHEET *sht;
    //修正 refresh的范围出了画面，进行修正
    if (vx0 < 0) { vx0 = 0; }
    if (vy0 < 0) { vy0 = 0; }
    if (vx1 > ctl->xsize) { vx1 = ctl->xsize; }
    if (vy1 > ctl->ysize) { vy1 = ctl->ysize; }
	for (h = 0; h <= ctl->top; h++) {
		sht = ctl->sheets[h];
		buf = sht->buf;
		/* 使用vx0~vy1，计算出bx0~by1，即相对位置，看是否重叠，重叠需要重置，不重叠直接刷新即可 */
		bx0 = vx0 - sht->vx0;
		by0 = vy0 - sht->vy0;
		bx1 = vx1 - sht->vx0;
		by1 = vy1 - sht->vy0;
		if (bx0 < 0) { bx0 = 0; }
		if (by0 < 0) { by0 = 0; }
		if (bx1 > sht->bxsize) { bx1 = sht->bxsize; }
		if (by1 > sht->bysize) { by1 = sht->bysize; }
		for (by = by0; by < by1; by++) {
			vy = sht->vy0 + by;
			for (bx = bx0; bx < bx1; bx++) {
				vx = sht->vx0 + bx;
				c = buf[by * sht->bxsize + bx];
				if (c != sht->col_inv) {
					vram[vy * ctl->xsize + vx] = c;
				}
			}
		}
	}
	return;
}





