/*
Author :zxfan 2015-01-31
FUNCTIONS:translate gb2312 to utf8
HIGH SPEED AND NEED NO FONT MEMORY
*/
#ifndef _GB2312TOUNICODE_H__
#define _GB2312TOUNICODE_H__

/*
function:�������gb2312��������תΪutf8��ʽ
argcs:1 ����2 ���
return:-1 argcs error,maybe null
	-2 you lose the font dat file
	0 success
*/
int Gb2312ToUTF(unsigned char *inputWord,unsigned char *outputWord);
#endif
