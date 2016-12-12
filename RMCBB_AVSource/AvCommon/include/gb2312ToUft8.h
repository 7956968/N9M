/*
Author :zxfan 2015-01-31
FUNCTIONS:translate gb2312 to utf8
HIGH SPEED AND NEED NO FONT MEMORY
*/
#ifndef _GB2312TOUNICODE_H__
#define _GB2312TOUNICODE_H__

/*
function:将输入的gb2312编码数据转为utf8格式
argcs:1 输入2 输出
return:-1 argcs error,maybe null
	-2 you lose the font dat file
	0 success
*/
int Gb2312ToUTF(unsigned char *inputWord,unsigned char *outputWord);
#endif
