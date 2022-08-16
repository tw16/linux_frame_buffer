// date:2022年 08月 15日 星期一 20:05:19 CST
// author: CD2206
// path: /mnt/hgfs/2206预科/04-文件IO/05-开发板/BMP
 
//标准IO
#include <stdio.h>
#include <string.h>
//系统IO
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mman.h>
#include <linux/fb.h>
#include <sys/ioctl.h>
//内存管理
#include <stdlib.h>
//错误码
#include <errno.h>

#define LCD_DEV "/dev/fb0"
#define LCD_SIZE 800*480*4


struct ctl_lcd
{
	int fd;
	char *fb;
	struct fb_var_screeninfo vinfo; //LCD硬件信息
};


int lcd_init(struct ctl_lcd *lcd)
{
	lcd->fd = open(LCD_DEV, O_RDWR);
	if(lcd->fd == -1)
	{
		printf("errno open =%d \n", errno);
		return errno; //返回出错原因
	}

	lcd->fb = mmap(NULL, LCD_SIZE, PROT_READ|PROT_WRITE, MAP_SHARED, lcd->fd, 0);
	if(lcd->fb == MAP_FAILED)
	{
		printf("errno mmap =%d \n", errno);
		return errno; //返回出错原因
	}
	
	if(ioctl(lcd->fd, FBIOGET_VSCREENINFO, &lcd->vinfo) == -1)
	{
		printf("errno ioctl =%d \n", errno);
		return errno;
	}
	
	return 0;
}


int lcd_close(struct ctl_lcd *lcd)
{
	munmap(lcd->fb, LCD_SIZE);
	if(close(lcd->fd) == -1)
		return errno;
	return 0;
}

 
int main(int argc, char *argv[])
{
	struct ctl_lcd *lcd = calloc(1, sizeof(struct ctl_lcd));

// printf("%s:%d______>\n", __FUNCTION__, __LINE__);
	if(lcd_init(lcd) != 0)
	{
		perror("lcd_init() failed");
		return -1;
	}
	
	
	printf("LCD transp offset: %d, length = %d, msb_right = %d\n", lcd->vinfo.transp.offset, lcd->vinfo.transp.length, lcd->vinfo.transp.msb_right);
	printf("LCD red offset: %d, length = %d, msb_right = %d\n", lcd->vinfo.red.offset, lcd->vinfo.red.length, lcd->vinfo.red.msb_right);
	printf("LCD green offset: %d, length = %d,msb_right = %d\n", lcd->vinfo.green.offset, lcd->vinfo.green.length, lcd->vinfo.green.msb_right);
	printf("LCD blue offset: %d, length = %d, msb_right = %d\n", lcd->vinfo.blue.offset, lcd->vinfo.blue.length, lcd->vinfo.blue.msb_right);
	
	
	
	int bmp_fd = open(argv[1], O_RDONLY);
	if(bmp_fd == -1)
	{
		perror("open bmp failed");
		return -1;
	}
	
	//读出信息头获取图片像素宽和像素高
	char bmp_info[54] = {0};
	read(bmp_fd, bmp_info, 54);
	
	unsigned int w = bmp_info[18] | bmp_info[19]<<8;
	unsigned int h = bmp_info[22] | bmp_info[23]<<8;
	unsigned int bmp_size = bmp_info[34] | bmp_info[35]<<8 | bmp_info[36]<<16 | bmp_info[37]<<24;
	unsigned long int size = ((((w*3)+3)>>2)<<2) * h;
	printf("w = %u, h = %u, w*h*3 = %u, bmp_size = %u, size = %lu\n", w, h, w*h*3, bmp_size, size);
	
	int row_size = w * 3;	//一行的字节数
	char *bmp_row = calloc(1, row_size+1);
	if(bmp_row == NULL)
	{
		perror("calloc failed");
		return -1;
	}
	
	int r = lcd->vinfo.red.offset/8;
	int g = lcd->vinfo.green.offset/8;
	int b = lcd->vinfo.blue.offset/8;
	printf("r=%d, g=%d, b=%d\n", r, g, b);
	
	char pad = 0;
	// bzero(&pad, 1);
	int ret = 0;
	int i, j;
	int h_off = 0;
	int w_off = 0;
	
	// ret = read(bmp_fd, bmp_row, row_size*h);
	// printf("%d line, 想要读到size = %d, 实际读到ret = %d\n", __LINE__, row_size*h, ret);
	
	// ret = read(bmp_fd, bmp_row, row_size);
	
	// if(ret == 0)
	// {
		// printf("没有多余\n");
		// return 0;
	// }
	// printf("%d line, 想要读到size = %d, 实际读到ret = %d\n", __LINE__, row_size*h, ret);
	
	// return 0;
	
	for(i=0; i<h; i++)	//多少行
	{
// printf("i = %d\n", i);
		bzero(bmp_row, row_size);
		ret = read(bmp_fd, bmp_row, row_size);
		bmp_row[row_size] = '\0';
		// bmp_row[row_size] = '\0';
// printf("strlen(bmp_row) = %d, row_size = %d, ret = %d\n", strlen(bmp_row), row_size, ret);
// printf("i = %d, row_size = %d, ret = %d, strlen(bmp_row) = %d\n", i, row_size, ret, strlen(bmp_row));
		for(j=0; j<w; j++)	//一行的第几个像素
		{
// printf("line = %d, j = %d\n", __LINE__, j+0);
			// memcpy (void * dest, const void *src, size_t n);
			memcpy(lcd->fb+(h-i-1+h_off)*800*4+w_off+j*4+b, bmp_row+j*3+0, 1);	//lcd->fb是一个char型指针
// printf("line = %d, j = %d\n", __LINE__, j+1);
			memcpy(lcd->fb+(h-i-1+h_off)*800*4+w_off+j*4+g, bmp_row+j*3+1, 1);
// printf("line = %d, j = %d\n", __LINE__, j+2);
			memcpy(lcd->fb+(h-i-1+h_off)*800*4+w_off+j*4+r, bmp_row+j*3+2, 1);
// printf("line = %d, j = %d\n", __LINE__, j+3);
			// memcpy(lcd->fb+(h-i-1)*800+j*4, bmp_row+j*3, 3);
			// memcpy(lcd->fb+(h-i-1)*800*4+j*4+3, &pad, 1);
// printf("line = %d, j = %d\n", __LINE__, j+3);
		}
	}
	
	//资源释放
	if(lcd_close(lcd) != 0)
	{
		perror("lcd_close() failed");
		return -1;
	}


	return 0;
}
