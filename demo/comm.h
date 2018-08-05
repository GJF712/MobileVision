#ifndef _COMM_H_
#define _COMM_H_

#include <stdio.h>
#include <stdbool.h>
#include <windows.h>

#define DEBUG_GYRO 0
#define DEBUG_CAMM 0

extern bool Exit_ProcessFlag;
DWORD WINAPI Comm_Process(LPVOID threadNum);

#endif