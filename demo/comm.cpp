
#include <conio.h>
#include <math.h>
#include <string.h>
#include "comm.h"

typedef union{
	UINT8 str[19];
	struct
	{
		UINT8 PRE;
		UINT8 TYPE;
		UINT16 LEN;
		UINT16 CRC;
		UINT8 ID;
		float Pitch;
		float Roll;
		float Yaw;
	}Gyro_Data_T;
}Gyro_Data_U;//字节对齐有问题

typedef union{
	UINT8 str[9];
	struct{
		float A;
		float B;
		UINT8 Sum;
	}Angle_Com_T;
}Angle_Com_U;

#define FiFilter_Counting	5

typedef struct
{
	UINT8 Counting;
	float FiFilter[FiFilter_Counting];
}FiFilter_T;

HANDLE hCom1;
HANDLE hCom2;

bool Exit_ProcessFlag = TRUE;

bool COM_Init(HANDLE *h_Com, char *Com_Name, int Band){
	*h_Com = CreateFileA(Com_Name,//COM1口
		GENERIC_READ | GENERIC_WRITE,//允许读和写
		0,//独占方式
		NULL,
		OPEN_EXISTING,//打开而不是创建
		0,//同步方式
		NULL);
	if (*h_Com == (HANDLE)-1)
	{
		printf("打开%s失败!\n", Com_Name);
		return FALSE;
	}
	else
	{
		printf("打开%s成功！\n", Com_Name);
	}
	SetupComm(*h_Com, 1024, 1024);//输入缓冲区和输出缓冲区的大小都是1024
	COMMTIMEOUTS TimeOuts;
	//设定读超时
	TimeOuts.ReadIntervalTimeout = 1000;
	TimeOuts.ReadTotalTimeoutMultiplier = 500;
	TimeOuts.ReadTotalTimeoutConstant = 5000;
	//设定写超时
	TimeOuts.WriteTotalTimeoutMultiplier = 500;
	TimeOuts.WriteTotalTimeoutConstant = 2000;
	SetCommTimeouts(*h_Com, &TimeOuts);//设置超时
	DCB dcb1;
	GetCommState(*h_Com, &dcb1);
	dcb1.BaudRate = Band;//波特率为9600
	dcb1.ByteSize = 8;//每个字节有8位
	dcb1.Parity = NOPARITY;//无奇偶校验位
	dcb1.StopBits = ONE5STOPBITS;//一个停止位
	dcb1.fParity = FALSE;
	dcb1.fNull = FALSE;
	SetCommState(*h_Com, &dcb1);
	PurgeComm(*h_Com, PURGE_TXCLEAR | PURGE_RXCLEAR);//清空缓冲区
	return TRUE;
}

static void crc16_update(UINT16 *currectCrc, const UINT8 *src, UINT16 lengthInBytes)
{
	UINT16 crc = *currectCrc;
	for (UINT8 j = 0; j < lengthInBytes; ++j)
	{
		UINT8 byte = src[j];
		crc ^= byte << 8;
		for (UINT8 i = 0; i < 8; ++i)
		{
			UINT16 temp = crc << 1;
			if (crc & 0x8000)
			{
				temp ^= 0x1021;
			}
			crc = temp;
		}
	}
	*currectCrc = crc;
}

UINT8 Check_Sum(UINT8 *Data, UINT8 len){
	UINT8 Sum = 0;
	while (len--){
		Sum += *Data++;
	}
	return Sum;
}

void Set_FiFilter(FiFilter_T *FiFilter, float Data)
{
	FiFilter->FiFilter[FiFilter->Counting++] = Data;
	if (FiFilter->Counting == FiFilter_Counting)
	{
		FiFilter->Counting = 0;
	}
}

float Get_FiFilter(FiFilter_T *FiFilter){
	UINT8 Count = 0;
	double Sum = 0;
	float Max = -32768, Min = 32767;
	while (Count < FiFilter_Counting)
	{
		Sum += FiFilter->FiFilter[Count];
		if (FiFilter->FiFilter[Count] > Max){
			Max = FiFilter->FiFilter[Count];
		}
		if (FiFilter->FiFilter[Count] < Min){
			Min = FiFilter->FiFilter[Count];
		}
		Count++;
	}
	return (float)((Sum - Max - Min) / (FiFilter_Counting - 2));
}

#define alpha 0//底座与水平面的夹角

#if DEBUG_GYRO == 0
DWORD WINAPI Comm_Process(LPVOID threadNum)
#else
int main(int argc, char *argv[])
#endif
{
	Gyro_Data_U Gyro_Data;
	float X = 0, Y = 0, Z = 0;
	DWORD rCount = 19;//读取的字节数
	DWORD wCount = 9;//写入的字节数
	Angle_Com_U Angle_Com;
	FiFilter_T FiFilter[3] = { { 0, }, { 0, }, { 0, } };

	char a[10], b[10];
	FILE *fpRead = fopen("com.txt", "r");
	if (fpRead == NULL)
	{
		return 0;
	}
	fscanf(fpRead, "%s", a);
	fscanf(fpRead, "%s", b);

	if (COM_Init(&hCom1, a, 115200) == FALSE){
		printf("打开读串口失败!\n");
		Exit_ProcessFlag = false;
	}

	if (COM_Init(&hCom2, b, 115200) == FALSE){
		printf("打开写串口失败!\n");
		Exit_ProcessFlag = false;
	}

	while (Exit_ProcessFlag)
	{
		if (!ReadFile(hCom1, Gyro_Data.str, rCount, &rCount, NULL))
		{
			printf("读串口失败!\n");
			break;
		}
		PurgeComm(hCom1, PURGE_TXCLEAR | PURGE_RXCLEAR);
		if ((Gyro_Data.Gyro_Data_T.PRE == 0x5A) &&
			(Gyro_Data.Gyro_Data_T.TYPE == 0xA5) &&
			(Gyro_Data.Gyro_Data_T.LEN == 0x0D) &&
			(Gyro_Data.Gyro_Data_T.ID == 0xD9)){
			UINT16 crc = 0;
			crc16_update(&crc, &Gyro_Data.Gyro_Data_T.PRE, 4);
			crc16_update(&crc, &Gyro_Data.Gyro_Data_T.ID, Gyro_Data.Gyro_Data_T.LEN);
			if (Gyro_Data.Gyro_Data_T.CRC == crc){
				X = *(float *)&Gyro_Data.str[7];

				Set_FiFilter(&FiFilter[1], *(float *)&Gyro_Data.str[15]);
				Y = Get_FiFilter(&FiFilter[1]);
				Angle_Com.Angle_Com_T.A = Y;

				Set_FiFilter(&FiFilter[2], *(float *)&Gyro_Data.str[11]);
				Z = Get_FiFilter(&FiFilter[2]);
				Angle_Com.Angle_Com_T.B = Z + alpha - Y * alpha / 90;

				Angle_Com.Angle_Com_T.Sum = Check_Sum(Angle_Com.str, 8);
				PurgeComm(hCom2, PURGE_TXCLEAR | PURGE_RXCLEAR);
				//strcpy((char *)Angle_Com.str, "STM32F030");
				if (!WriteFile(hCom2, Angle_Com.str, wCount, &wCount, NULL))//前四个字节对应的是水平面旋转角度，后面跟随的是垂直平面的旋转角度
				{
					printf("写串口失败!\n");
					break;
				}
			}
		}
		Sleep(10);
	}
	Exit_ProcessFlag = false;
	CloseHandle(hCom1);
	CloseHandle(hCom2);

	return 0;
}
