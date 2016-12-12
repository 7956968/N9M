#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/time.h>
#include <string.h>
#include "gb2312ToUft8.h"

typedef struct fonthead_tag
{
	int wordNum;//×Ö¸öÊý
	unsigned short start_code;//gb2312 code
	unsigned short end_code;//gb2312 code
}FontHead;


unsigned short findunicode(int fd,unsigned short input,int start,int end)
{
	int temp = 0;
	unsigned short temp_value[2];

	if(start > end)
	{
		printf("parameter error[%04X] ----\n",input);
		return 0;	
	}

	temp = start +(end-start)/2;

	if(fd <= 0)
	{
		printf("FONT UNINIT\n");
		return 0;
	}
	
	lseek(fd,temp*4 + sizeof(FontHead),SEEK_SET);
	read(fd,temp_value,4);

	if(temp_value[0] > input)
	{
		return findunicode(fd,input,start,temp-1);
	}
	else if(temp_value[0] < input)
	{
		return findunicode(fd,input,temp + 1,end);
	}
	else
	{
		return temp_value[1];
	}
}

int UnicodeToUTF(unsigned long unicode, unsigned long &UTF8Value, int &utf8_byte_num)
{
	unsigned char ByteNum = 0;
	int count = 0;
	
	if (unicode >= 0x0 && unicode <= 0x7F)
	{
		ByteNum = 1;
	}
	else if (unicode >= 0x80 && unicode <= 0x7FF)
	{
		ByteNum = 2;
	}
	else if (unicode >= 0x800 && unicode <= 0xFFFF)
	{
		ByteNum = 3;
	}
	else if (unicode >= 0x10000 && unicode <= 0x10FFFF)
	{
		ByteNum = 4;
	}
	else
	{
		printf("UnicodeToUTF::invalid unicode[%lx]\n", unicode);
		return false;
	}

	utf8_byte_num = ByteNum;
	count = ByteNum;
	*((char *)&UTF8Value) = (0xF >> (4 - ByteNum)) << (8 - ByteNum);
	
	while (count)
	{	
		if (count == 1)
		{
			*((char *)&UTF8Value) |= (unicode >> ((ByteNum - count) * 6));
		}
		else
		{
			*(((char *)&UTF8Value) + count - 1) = ((unicode >> ((ByteNum - count) * 6)) & 0x3F) | 0x80;
		}
		
		count--;
	}
	return true;
}


int Gb2312ToUTF(unsigned char *inputWord,unsigned char *outputWord)
{
	if((inputWord == 0)||(outputWord == 0))
	{
		return -1;
	}

	char *fontPath=(char*)"/usr/local/app/plugin/gb2312tounicode.dat";
	FontHead fonthead;
	memset(&fonthead,0x0,sizeof(FontHead));
	int fd = open(fontPath,O_RDONLY);
	if(fd < 0)
	{
		fd = open(fontPath,O_RDONLY);
		if(fd < 0)
		{
			printf("WE NEED FONT DAT FILE here NAME[gb2312tounicode.dat]  %s\n",fontPath);
			return -2;
		}
	}
	
	read(fd,&fonthead,sizeof(FontHead));
	int len = 0;
	unsigned long utf8 = 0;
	int utf8_byte_num;
	unsigned long unicode = 0;
	unsigned short gb2312 = 0;
	//for the test
//	unsigned char nameStr[4]= {0};

#if 0
	printf("\n####inputWord ------- \n");
	for(int w = 0;w<strlen((char*)inputWord);w++)
		printf("%02X ",inputWord[w]);
	printf("\n####inputWord -------\n");
#endif	
	for(int i=0;i<(int)strlen((char*)inputWord);)
	{
		if ((unsigned char)inputWord[i] < (unsigned char)0x80)
		{
			memcpy(&outputWord[0] + len, &inputWord[i], 1);
			i++;
			len ++;
		//	printf("DO NOT TRANS %d--[%02X]\n",i,inputWord[i]);
		}
		else
		{
			gb2312 = (inputWord[i]*256) + (inputWord[i+1]);
			i += 2;
			#if 0
			/*for the test*/
			printf("gb2312 --- %04X\n",gb2312);
			memset(nameStr,0x0,4);
			nameStr[0] = 0xc4;
			nameStr[1] = 0xe3;
			printf("nameStr ---- %s\n\n",nameStr);
			#endif
			/*for the test*/			
			if((gb2312 >= fonthead.start_code)&&(gb2312 <= fonthead.end_code))
			{
				unicode = findunicode(fd,gb2312,0,fonthead.wordNum -1);
				if(unicode != 0)
				{
				//	printf("GBCODE[%04X] --- UNICODE[%04X]\n",gb2312,unicode);
					UnicodeToUTF(unicode, utf8, utf8_byte_num);
					memcpy(&outputWord[0] + len, &utf8, utf8_byte_num);
					len = len + utf8_byte_num;
				}
			}
			else
			{
				/*not in my table  */
				printf("FIND A UNKNOWN WORD PLEASE ADD IN GBCODE[%04X]### FANTASY THANKS YOU\n",gb2312);
				memcpy(&outputWord[0] + len,&gb2312,2);
				len += 2;
			}
		}
	
	}
	close(fd);
	return 0;
}

