#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<fcntl.h>
#include<linux/ioctl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <string.h>
#include <sapi.h>
//#include"function.h"

unsigned int flag=0;
unsigned int Code_Num_01 = 0;
unsigned int Code_Num_Start[15];
unsigned int Code_Num_NewStart[15];
unsigned int Data_Len = 0;
unsigned int If_Read = 0;
unsigned int Sec_Num = 0;//section number
unsigned int Sec_Num_Length;
unsigned int Sec_Len[15]; //every data section length
unsigned int Sec_Len_Con[15];
unsigned int check_code;//校验和
char Code_Source_01[100000];   //去掉‘ ’，‘’
char Code_Source_02[100000];
static unsigned short Code_Source_Con[100000]; 
unsigned short Code_Source_Con01[100000];
unsigned short Code_Source_Con02[100000];
unsigned char version[10];

unsigned int feedBack;
unsigned int feedBackCount = 0;
int dev_fd;

//加载需要的定义
//typedef struct rdwr_node{
//	unsigned short offset;
//	unsigned short data;
//}rdwr_node, *prdwr_node;

//Read&Write reg interface
int write_422_reg(int devfd, rdwr_node *node);
int read_422_reg(int devfd, rdwr_node *node);
int read_IO_reg(int devfd, rdwr_node *node);
int write_IO_reg(int devfd, rdwr_node *node);
int read_AD_reg(int devfd, rdwr_node *node);
int write_AD_reg(int devfd, rdwr_node *node);


//从光驱中读文件
int readfile_from_cd(char *file_path0);
void chartoint(char arr[], int length);
void chartoint2(char arr[], int length);
void Confirm_Product();
void ClearOverAV(unsigned short data);
void EnableAD(unsigned short data);
int SendData(unsigned short data);
unsigned short RecvData();
int ClearSendFifo();
int ClearRecvFifo();
void ClearACFifo(unsigned short data);
unsigned short RecvFifoCount();
void OnPowerCtl();
void DownPowerCtl();
unsigned short ReadIntStatus();
void Read_Version();
void Self_Check();
unsigned short Read_AV();
void sload();
unsigned int check_ok();


//#define FILE_PATH "/home/gao/sload/otest"
//#define SOURCE_FILE_PATH "/home/gao/sload/prog.a00"
//#define SOURCE_FILE_PATH "/home/gao/sload/a00.txt"
#define FLASH_WAIT_TIME 60
#define dev "/dev/XC422"
#define XC422_BUS_TYPE 0x70
#define IOCTL_READ_REG422 _IOWR(XC422_BUS_TYPE,0x02,rdwr_node)
#define IOCTL_WRITE_REG422 _IOWR(XC422_BUS_TYPE,0x01,rdwr_node)

#define IOCTL_WRITE_REGIO	_IOWR(XC422_BUS_TYPE, 0x05, rdwr_node)
#define IOCTL_READ_REGIO _IOWR(XC422_BUS_TYPE, 0x06, rdwr_node)

#define IOCTL_WRITE_REGAD	_IOWR(XC422_BUS_TYPE, 0x03, rdwr_node)
#define IOCTL_READ_REGAD _IOWR(XC422_BUS_TYPE, 0x04, rdwr_node)

unsigned int Start_Command[9] = { 0x75, 0x86, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x86 };
//unsigned int Start_Command[9] = { 0x75, 0x88, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0xAA, 0x88 };
//unsigned int Start_Commandtemp[9] = { 0x75, 0x86, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x86 };
//unsigned int Start_Commandrec[9];
time_t timeBegin, timeEnd;
rdwr_node node;



//******************serial load**********************
void sload()
{
	if (flag == 0)
	{
		return;
	}
	int i = 0;
	int count = 0;
	unsigned short j;
//	EnableAD(0x1);
	ClearRecvFifo();
	printf("before send count:%d\n",RecvFifoCount());
	if (If_Read == 0)//file is read or is not read
	{
		send_tbuf("Please Read file\r\n");
		return;
	}
	for (i = 0; i < Data_Len; i++)
	{
		printf("send data1:%x\n",Code_Source_Con01[i]);
		if (!SendData((Code_Source_Con01[i]) & 0xFF))
		{
			//			printf("send success\n");
		}
		else{

			printf("send fail!\n");
			send_tbuf("send fail!\r\n");
			DownPowerCtl();
			break;
//			exit(0);
		}
		timeBegin = time(NULL);//start time
		usleep(10);
		while (1)
		{
			timeEnd = time(NULL);
			if (1 == RecvFifoCount())
			{
				break;

			}
			else{
				if ((timeEnd - timeBegin)>FLASH_WAIT_TIME)
				{
					printf("OVER TIME!\n");
					send_tbuf("OVER TIME!\r\n");
					DownPowerCtl();
					break;
				//	exit(0);
				}
			}

		}

		j = (RecvData() & 0xFF);
		printf("j:%x\n",j);
		if (Code_Source_Con01[i] == j)
		{
			count++;
			continue;
		}
		else{
			printf("Receive DATA ERROR!!\n");
			send_tbuf("Receive DATA ERROR!!\r\n");
			printf("No.%d data ERROR!\n", count);
			printf("send data2:%x\n",Code_Source_Con01[i]);
			printf("rev data:%x\n",RecvData());
		//	DownPowerCtl();
			//exit(0);
			return;
		}
	}

	printf("count of success %d\n", count);
//	EnableAD(0x0);
//	DownPowerCtl();

}
//**********************read data from cd ******************************************
int readfile_from_cd(char *file_path0)
{
	printf("read file from cd begin...\n");
	int in, out;

	in = open(file_path0, O_RDONLY);
	if (in < 0)
	{
		printf("open in failed\n");
		return -1;
	}

	unsigned int fsize;
	unsigned int rsize;
	unsigned int wsize;
	unsigned int wfsize;
	unsigned int i,j,k,m,n;
	struct stat statbuff;
	if (stat(file_path0, &statbuff) < 0)
		return -1;
	fsize = statbuff.st_size;

	unsigned char *Code_Source = (unsigned char *)malloc((fsize)*sizeof(unsigned char));

	rsize = read(in, Code_Source, fsize);
	printf("start clean data\n");

	for (i = 0; i < rsize; i++)
	{
		if ((Code_Source[i]!=0x2)&&(Code_Source[i]!=0x3)&&(Code_Source[i]!=' ')&&(Code_Source[i]!=',')&&(Code_Source[i]!='-')&&(Code_Source[i]!='\r')&&(Code_Source[i]!='\n'))
		{
			Code_Source_01[Code_Num_01] = Code_Source[i];
			Code_Num_01 = Code_Num_01 + 1;
		}
	}
	printf("after cleaning-------------------------------------------------ok\n");
	//靠靠靠靠靠靠
	j = 0;
	for (i = 0; i < Code_Num_01; i++)//Code_Num_01 count after clear data.
	{
		if ((Code_Source_01[i] == '$') && (Code_Source_01[i + 1] == 'A'))
		{
			Code_Num_Start[j] = i;
			j = j + 1;
		}
		else if ((Code_Source_01[Code_Num_01] == '$') && (Code_Source_01[Code_Num_01 + 1] != 'A'))
		{
			printf("Data in file ERROR!!");
		}
	}
	//********************************
	printf("sec count:---------j=%d----Code_NUm_01=%d\n",j,Code_Num_01);
	for (k = 0; k < j; k++)
	{
		printf("sec count%d\n", Code_Num_Start[k]);
	}
	

	Code_Num_Start[j] = Code_Num_01;
	Sec_Num = j;	
	j=0;
	i=0;
	for(j=0;j<Sec_Num;j++)
	{
		Sec_Len[j] = ((Code_Num_Start[i + 1] - Code_Num_Start[i]) - 8)/4;	//产品数据的单位是一个字（两个字节）
	}
	
	j = 0;
	for (i = 0; i < Code_Num_01; i++)
	{
		if (Code_Source_01[i] == '$')
		{
			i = i + 1;
			Code_Source_02[j] = '0';
			j = j + 1;
			Code_Source_02[j] = '0';
			j = j + 1;
		}
		else{
			Code_Source_02[j] = Code_Source_01[i];//
			j=j+1;
		}
	}
	printf("finnaldata-----------j=%d-----------------\n",j);
	//Data_Len = j/2;

	for(i=0;i<Sec_Num;i++)
	{
		Sec_Len_Con[i]=(Code_Num_Start[i+1]-Code_Num_Start[i]-8)/2;
	}
	Code_Num_NewStart[0] = 0;
	for(i=1;i<Sec_Num;i++)
	{
		Code_Num_NewStart[i] = Code_Num_NewStart[i-1]+Sec_Len_Con[i-1]+4;
	}

	for(i=0;i<Sec_Num;i++)
	{
		printf("Sec_Len_Con%d-",Sec_Len_Con[i]);
	
	}
	printf("\n");
	
	chartoint(Code_Source_02,j);
	//get final format data
	j = 0;
	k = 0;
	for (i = 0; i < Sec_Num; i++)//整理成最后的数据
	{
		unsigned short temp;
		m = Code_Num_NewStart[i]+4;//No.i data start
		n = Code_Num_NewStart[i];
		//sec[i] address low and high
		temp = Sec_Len_Con[i] & 0xFF;
	//	printf("temp1::%d\n",temp);
		Code_Source_Con01[k++] = temp;
		temp = Sec_Len_Con[i] >> 8;
		temp = temp & 0xFF;
	//	printf("temp2::%d\n",temp);
		Code_Source_Con01[k++] = temp;
		//sec[i] data start address
		
		Code_Source_Con01[k++] = Code_Source_Con[n + 1];
		Code_Source_Con01[k++] = Code_Source_Con[n];
		Code_Source_Con01[k++] = Code_Source_Con[n+3];
		Code_Source_Con01[k++] = Code_Source_Con[n + 2];
	
	
		//sec[i] data
		for (j = 0; j < Sec_Len_Con[i]; )
		{
			Code_Source_Con01[k++] = Code_Source_Con[m + 1];
			Code_Source_Con01[k++] = Code_Source_Con[m];
			m = m + 2;
			j = j + 2;
		}
		
	}
	for (i = 0; i<k; i++)
	{
		printf("%x-", Code_Source_Con01[i]);
	}
	If_Read = 1;
	if (0 != (k % 2))
	{
		Code_Source_Con01[k] = 0;
		Data_Len = k + 1;
	}
	else{
		Data_Len = k;
	}
	send_tbuf("Read File Completed!");
	return 0;
}

//****************校验代码********************
unsigned int check_ok()
{
	int i = 0;
	check_code = 0;
	for (i = 0; i < Data_Len;i++)
	{
		check_code = (Code_Source_Con01[i] + check_code) & 0xFFFF;
		////int temp = Sec_Len_Con[i];
		//if (i == Code_Num_NewStart[i])
		//{
		//	Code_Source_Con02[k] = (Sec_Len[j] / 256) & 0xFF;
		//	k = k + 1;
		//	Code_Source_Con02[k] = (Sec_Len[j] % 256) & 0xFF;
		//	k = k + 1;
		//	Code_Source_Con02[k] = Code_Source_Con[i];
		//	k = k + 1;
		//	j = j + 1;
		//}
		//else{
		//	Code_Source_Con02[k] = Code_Source_Con[i];
		//	k = k + 1;
		//}
	}
	return check_code;
}
//***************Cleae voltage & current fifo**********
void ClearACFifo(unsigned short data)
{
	node.offset = 0x4;
	node.data = data;
	write_AD_reg(dev_fd, &node);
	//printf("clearACFifo and wait 1s\n");
}
//*****************Read_AV************************
unsigned short Read_AV()
{
	float Voltage, Current;
	unsigned short i,vol_cur;
	
//	ClearACFifo(0x1);
//	OnPowerCtl();
	EnableAD(0x1);
	usleep(1000);

	node.offset = 0x1;
	read_AD_reg(dev_fd, &node);
//	printf("enable reg :%x\n",node.data);
	char temp[10];
	char sBuf[10];
	for (i = 0; i < 2; i++)
	{
		node.offset = 0x2;
//		OnPowerCtl();
		read_AD_reg(dev_fd, &node);
		//printf("%x\n", node.data);
		vol_cur = node.data;

		if (((vol_cur >> 11) & 0x7) == 0x0)
		{
			Voltage = (((float)((node.data >> 1) & 0x3FF) * 5) / 1024) * 12;
//			printf("VVVVVVVV:%f-%x\n", Voltage, (node.data >> 15));
			//temp = (char*)(&Voltage);
			sprintf(temp,"%f",Voltage);
			sBuf[0] = temp[0];
			sBuf[1] = temp[1];
			sBuf[2] = temp[2];
			sBuf[3] = temp[3];
			sBuf[4] = temp[4];
			sBuf[5] = temp[5];
			sBuf[6] = '\0';
			send_tbuf_v(sBuf);
//			send_tbuf_v("4321");
		}
		else if (((vol_cur >> 11) & 0x7) == 0x1)
		{
			Current = (((float)((node.data >> 1) & 0x3FF) * 5) / 1024) / 3.5;
//			printf("AAAAAAAA:%f-%x\n", Current, (node.data >> 15));
			//temp = (char*)(&Current);
			sprintf(temp,"%f",Current);
			sBuf[0] = temp[0];
			sBuf[1] = temp[1];
			sBuf[2] = temp[2];
			sBuf[3] = temp[3];
			sBuf[4] = temp[4];
			sBuf[5] = temp[5];
			sBuf[6] = '\0';
			send_tbuf_a(sBuf);
//			send_tbuf_a("1234");
		}
		else{
			printf("AV Read ERROR!\n");
		}
	}
	
	EnableAD(0x0);
	
	return;
}

//*******************Read_Version*************************
void Read_Version()
{
	OnPowerCtl();
	sleep(2);
	ClearSendFifo();
	ClearRecvFifo();
	unsigned short Version[9] = { 0x75, 0x85, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x85 };
	unsigned short Version_Feed[9]={0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0,0x0};
	int i;
	for (i = 0; i < 9; i++)
	{
		node.data = Version[i];
		if (0 != SendData(node.data))
		{
			printf("Read_Version SendData error!\n");
		}
	}
	for(i=0;i<9;i++)
	{
		printf("%x-",Version[i]);
	}
	printf("\n");
	sleep(1);
	printf("version_feed count::%d\n",RecvFifoCount());
	for (i = 0; i < 9; i++)
	{
		Version_Feed[i] = RecvData();
	}
	for(i=0;i<9;i++)
	{
		printf("%x-",Version_Feed[i]);
	}
	if (Version_Feed[0] == 0x75 && Version_Feed[1] == 0x85)
	{
		printf("Version %d.%d\n", Version_Feed[3], Version_Feed[4]);
		unsigned char temp_version[3];
	//	temp_version[0] = Version_Feed[3];
//		itoa(Version_Feed[3],&temp_version[0],10);
		sprintf(&temp_version[0],"%x",Version_Feed[3]);
		temp_version[1] = '.';
		sprintf(&temp_version[2],"%x",Version_Feed[4]);
//		itoa(Version_Feed[4],&temp_version[2],10);
	//	temp_version[2] = Version_Feed[4];
		unsigned char buf[15] = "Version:";
		strcat(buf, temp_version);
		strcat(buf,"\r\n");
		send_tbuf(buf);
	}
	else{
		printf("Version Feedback ERROR!\n");
	}
	DownPowerCtl();
}

//*******************change char array to int***************************************
void chartoint2(char arr[],int length)
{
	int i,j=0;
	char temp[3];


	for(i=0;i<length;)
	{
//		temp[0]='\0';
//		temp[1]='\0';
		temp[2]='\0';
		if((arr[i]==EOF)||(arr[i+1]==EOF))
			return;
//		temp[0] = arr[i];
//		temp[1] = arr[i+1];
		temp[0]='F';
		temp[1]='A';
//		printf("%c-%c***",temp[0],temp[1]);
		sscanf(temp,"%x",Code_Source_Con+j);
//		*(Code_Source_Con+j)=atoi(temp);
//		printf("--------------atoi--------------\n");
//		printf("%x-",atoi(temp));
		i=i+2;
		j=j+1;
	}
}

//*******************change char array to int******************************************
void chartoint(char arr[], int length)
{
	int i,j=0;
	for(i=0;i<length;)
	{
		//two number
		if ((arr[i] >= 48) && (arr[i] <= 57) && (arr[i + 1] >= 48) && (arr[i + 1] <= 57))
		{
			Code_Source_Con[j] = (unsigned short)((arr[i] - 48) * 16 + arr[i + 1] - 48);
		}
		else if ((arr[i] >= 48) && (arr[i] <= 57) && (arr[i + 1] >= 65) && (arr[i + 1] <= 70))//num let
		{
			Code_Source_Con[j] = (unsigned short)((arr[i] - 48) * 16 + arr[i + 1] - 55);
		}
		else if ((arr[i] >= 65) && (arr[i] <= 70) && (arr[i + 1] >= 48) && (arr[i + 1] <= 57))
		{
			Code_Source_Con[j] = (unsigned short)((arr[i] - 55) * 16 + arr[i + 1] - 48);
		}
		else if ((arr[i] >= 65) && (arr[i] <= 70) && (arr[i + 1] >= 65) && (arr[i + 1] <= 70))
		{
			Code_Source_Con[j] = (unsigned short)((arr[i] - 55) * 16 + arr[i + 1] - 55);
		}
		else if ((arr[i] >= 97) && (arr[i] <= 102) && (arr[i + 1] >= 65) && (arr[i + 1] <= 70))
		{
			Code_Source_Con[j] = (unsigned short)((arr[i] - 87) * 16 + arr[i + 1] - 55);
		}
		else if ((arr[i] >= 97) && (arr[i] <= 102) && (arr[i + 1] >= 48) && (arr[i + 1] <= 57))
		{
			Code_Source_Con[j] = (unsigned short)((arr[i] - 87) * 16 + arr[i + 1] - 48);
		}
		else if ((arr[i] >= 65) && (arr[i] <= 70) && (arr[i + 1] >= 97) && (arr[i + 1] <= 102))
		{
			Code_Source_Con[j] = (unsigned short)((arr[i] - 55) * 16 + arr[i + 1] - 87);
		}
		else if ((arr[i] >= 48) && (arr[i] <= 57) && (arr[i + 1] >= 97) && (arr[i + 1] <= 102))
		{
			Code_Source_Con[j] = (unsigned short)((arr[i] - 48) * 16 + arr[i + 1] - 87);
		}
		else if ((arr[i] >= 97) && (arr[i] <= 102) && (arr[i + 1] >= 97) && (arr[i + 1] <= 102))
		{
			Code_Source_Con[j] = (unsigned short)((arr[i] - 87) * 16 + arr[i + 1] - 87);
		}
		else{
			if ((arr[i] != EOF) && (arr[i + 1] != EOF)){
				printf("DATA ERROR!!");
			}		
		}
		i=i+2;
		j=j+1;
	}
}


//**************upLoad data to product******************************

void Confirm_Product()
{
	flag = 0;
	OnPowerCtl();
	sleep(1);
	ClearSendFifo();
	ClearRecvFifo();
	int i, j, k;
	time_t timeBegin, timeEnd;
	if (If_Read == 0)//file is read or is not read
	{
		printf("ERROR!!File IS NOT READ!!\n");
		send_tbuf("ERROR!!File IS NOT READ!!\r\n");
		return;
	}

	for (i = 0; i < 9; i++)
	{
		node.data = Start_Command[i];
		SendData(node.data);
	}
	timeBegin = time(NULL);//start time
	while (1)
	{
		timeEnd = time(NULL);
		if ((timeEnd - timeBegin) > FLASH_WAIT_TIME)
		{
			if (feedBackCount == 0)
			{
				printf("Start_Command feedback overtime!!\n");
				send_tbuf("Start_Command feedback overtime!!\r\n");
				DownPowerCtl();
				return;
			}
			else{
				printf("Erase flash OverTime!!!!!\n");
				send_tbuf("Erase flash OverTime!!!!!\r\n");
				DownPowerCtl();
				return;
			}
		}
		if (1 == RecvFifoCount())
		{
			if (0xAA == (RecvData() & 0xFF))
			{
				feedBackCount++;
				printf("erasing flash!!\n");
				send_tbuf("erasing flash!!\r\n");
			}
			else{
				printf("Start_Command ERROR!!!!!\n");
				send_tbuf("Start_Command ERROR!!!!!\r\n");
				DownPowerCtl();
				//exit(0);
				return;
			}
			if (2 == feedBackCount)
			{
				printf("feedback success! start loading file!!\n");
				send_tbuf("feedback success! start loading file!!\r\n");
				break;
			}
		}
			
	}
	flag = 1;
//	DownPowerCtl();

}

//*************Self Check********************
void Self_Check()
{
	OnPowerCtl();
	sleep(2);
	ClearSendFifo();
	ClearRecvFifo();
	int i, j, k;
	int Time=0;
	int Check_Flag = 0;
	unsigned short Version[9] = { 0x75, 0x81, 0x0, 0x0, 0x0, 0x0, 0x0, 0x44, 0xC5 };
	unsigned short Version_Feed[27];
	unsigned char buf[25]="\0";
	unsigned char temp[3];

	for (i = 0; i < 9; i++)
	{
		node.data = Version[i];
		if(0!=SendData(node.data))
		{
			printf("send data error!\n");
		}
		printf("%x-",Version[i]);
	}
	printf("\n");
	printf("version send completed!\n");
	//sleep(10);
	//printf("count-%x\n",RecvFifoCount());
	while (1)
	{
		if (Time>100000)
		{
			printf("Receive Self Check Fail!!\n");
			DownPowerCtl();
			return;
		}
		Time=Time+1;
//		sleep(1);
		if (RecvFifoCount() >= 0x1b)
		{
			for (i = 0; i < 27; i++)
			{
				Version_Feed[i] = RecvData();
			}
			/*for(i=0;i<27;i++)
			{
				printf("%x-",Version_Feed[i]);
			}*/
			if ((Version_Feed[0] == 0x75) && (Version_Feed[1] == 0x81))
			{
				for (k = 1; k < 8; k++)
				{
					Check_Flag = (Version_Feed[k] ^ Check_Flag) & 0xFF;
				}
				if (Check_Flag == Version_Feed[8])
				{
					if ((Version_Feed[2] & 0xF0) == 0xA0)
					{
						for (j = 0; j < 27; j++)
						{
							printf("%x-", Version_Feed[j]);
							if ((j == 8) || (j == 17))
							{
								printf("\n");
							}
						}
						printf("Self Check OK!\n");
						for (i = 0; i < 27; i++)
						{
							sprintf(temp, "%x", Version_Feed[i]&0xff);
							//send_tbuf(buf);
							strcat(buf, temp);
							strcat(buf, " ");
							if ((i == 8) || (i == 17))
							{
								strcat(buf, "\r\n");
							
							//	memset(buf, '\0', sizeof(buf));
							}
						}
						
						send_tbuf("\r\n");
						send_tbuf("Self Check OK!\r\n");
					}
					else{
						for (j = 0; j < 27; j++)
						{
							printf("%x-", Version_Feed[j]);
							if ((j == 8) || (j == 17))
							{
								printf("\n");
							}
						}
						printf("Self Check FAIL!\n");
						for (i = 0; i < 27; i++)
						{
							sprintf(temp, "%x", Version_Feed[i] & 0xff);
							//send_tbuf(buf);
							strcat(buf,temp);
							strcat(buf," ");
							
							if ((i == 8) || (i == 17))
							{
								strcat(buf, "\r\n");
							}
						}
						send_tbuf(buf);
						send_tbuf("\r\n");
						send_tbuf("Self Check FAIL!------------\r\n");
					}
					break;
				}
			}
			else{
				printf("Recv Version Head ERROR!\n");
			}
		}	
	}
	DownPowerCtl();
}

void ClearOverAV(unsigned short data)
{
	node.offset = 0x3;
	node.data = data;
	write_AD_reg(dev_fd, &node);
}
void EnableAD(unsigned short data)
{
	node.offset = 0x1;
	node.data = data;
	write_AD_reg(dev_fd, &node);
}

int SendData(unsigned short data){

	
	node.offset = 0x2;
	node.data = data;
	int ret = write_422_reg(dev_fd, &node);
	return ret;

}

unsigned short RecvData()
{
	
	node.offset = 0x3;
	read_422_reg(dev_fd, &node);
	return node.data;
}

int ClearSendFifo()
{

	node.offset = 0x6;
	node.data = 0x1;
	int ret = write_422_reg(dev_fd, &node);
	return ret;
}

int ClearRecvFifo()
{
	
	node.offset = 0x7;
	node.data = 0x1;
	int ret = write_422_reg(dev_fd, &node);
	return ret;
}
//
unsigned short RecvFifoCount()
{
	
	node.offset = 0x8;
	read_422_reg(dev_fd, &node);
	int count = node.data;
	return count;
}
//on power
void OnPowerCtl()//1:onPower  0:outPower
{
	node.offset = 0x0;
	node.data = 0x1;
	write_IO_reg(dev_fd, &node);
	printf("On Power!\n");
	
}
//down power
void DownPowerCtl()//1:onPower  0:outPower
{
	
	node.offset = 0x0;
	node.data = 0x0;
	write_IO_reg(dev_fd, &node);
	printf("Down Power!\n");
	
}
//read fifo status
unsigned short ReadIntStatus()
{
	
	node.offset = 0x1;
	read_IO_reg(dev_fd, &node);
	unsigned short Mask = 0x2;
	unsigned short val = node.data&Mask;
	return val;
}

//write 422 reg
int write_422_reg(int devfd, rdwr_node *node)
{
	int ret = ioctl(devfd, IOCTL_WRITE_REG422, node);

	return ret;
}

//read 422 reg
int read_422_reg(int devfd, rdwr_node *node)
{
	int ret = ioctl(devfd, IOCTL_READ_REG422, node);
	return ret;
}

//read IO reg
int read_IO_reg(int devfd, rdwr_node *node)
{
	int ret = ioctl(devfd, IOCTL_READ_REGIO, node);
	return ret;
}
//write IO reg
int write_IO_reg(int devfd, rdwr_node *node)
{
	int ret = ioctl(devfd, IOCTL_WRITE_REGIO, node);

	return ret;
}

//read AD reg
int read_AD_reg(int devfd, rdwr_node *node)
{
	int ret = ioctl(devfd, IOCTL_READ_REGAD, node);
	return ret;
}
//write AD reg
int write_AD_reg(int devfd, rdwr_node *node)
{
	int ret = ioctl(devfd, IOCTL_WRITE_REGAD, node);

	return ret;
}
