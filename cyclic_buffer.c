/**
 * FILE: cyclic_buffer.c
 * DESC: 循环缓冲区实现文件
 * DATE: 2018/3/01
 * AUTHOR: 高石林
 */
#include <stdio.h>
#include <cyclic_buffer.h>
//#include <common.h>
#include <errno.h>
#include <stdlib.h>

int init_buffer(pcyclic_buffer p, unsigned int buf_len)
{
	if(p == NULL || buf_len <= 0){
		errno = EINVAL;
		return -1;
	}
	
	if((p->buffer = malloc(buf_len)) == NULL)
		return -1;
	p->buf_len = buf_len;
	p->ptr1 = 0;
	p->ptr2 = 0;

	return 0;
}

int release_buffer(pcyclic_buffer p)
{
	if(p == NULL){
		errno = EINVAL;
		return -1;
	}
	if(p->buffer != NULL)
		free(p->buffer);
	return 0;
}

int is_buffer_empty(pcyclic_buffer p)
{
	return ((p->ptr1 == p->ptr2) ? 1 : 0);
}

int is_buffer_full(pcyclic_buffer p)
{
	return !is_buffer_empty(p);
}

int get_buffer_len(pcyclic_buffer p)
{
//	printf("get  buffer\n");
	if(p->ptr1 >= p->ptr2)
		return (p->ptr1 - p->ptr2);
	else
		return (p->ptr1 + p->buf_len - p->ptr2);

	
}

int get_buffer_free(pcyclic_buffer p)
{
	int retval;
	retval = get_buffer_len(p);
	return (p->buf_len - retval - 1);
}

void remove_bytes(pcyclic_buffer p, int len)
{
	p->ptr2 = (p->ptr2 + len) % p->buf_len;
}

void add_bytes(pcyclic_buffer p, unsigned char *src, int len)
{
	unsigned int index, i;
	index = p->ptr1;
	for(i = 0; i < len; i++){
		p->buffer[index] = src[i];
		index = (index + 1) % p->buf_len;
	}
	p->ptr1 = index;
}

int get_int32(pcyclic_buffer p)
{
	unsigned int i1, i2, i3, i4;
	
	i1 = p->ptr2;
	i2 = (i1 + 1) % p->buf_len;
	i3 = (i2 + 1) % p->buf_len;
	i4 = (i3 + 1) % p->buf_len;
	p->ptr2 = (p->ptr2 + 4) % p->buf_len;

	return ((p->buffer[i1]) | ((p->buffer[i2] & 0xFF) << 8)|
		((p->buffer[i3] & 0xFF) << 16) |
		((p->buffer[i4] &0xFF) << 24));
}
short int get_int16(pcyclic_buffer p)
{
	unsigned int i1, i2;
	
	i1 = p->ptr2;
	i2 = (i1 + 1) % p->buf_len;
	p->ptr2 = (p->ptr2 + 2) % p->buf_len;

	return ((p->buffer[i1]) | ((p->buffer[i2] & 0xFF) << 8));
}

unsigned char get_byte(pcyclic_buffer p)
{
	unsigned char byte;

	byte = p->buffer[p->ptr2];
	p->ptr2 = (p->ptr2 + 1) % p->buf_len;

	return byte;
}
void copy_bytes(pcyclic_buffer p, unsigned char *buf, int len)
{
	unsigned int index, i;
	
	if(p == NULL || get_buffer_len(p) < len){
		return;
	}
	index = p->ptr2;
	for(i = 0; i < len; i++){
		buf[i] = p->buffer[index];
		index = (index + 1) % p->buf_len;
	}
}
