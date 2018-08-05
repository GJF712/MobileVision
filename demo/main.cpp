#include <opencv2\opencv.hpp>
#include "comm.h"
#include <stdint.h>

using namespace std;
using namespace cv;

#if DEBUG_GYRO == 0
int main(int argc, char *argv[])
{
#if DEBUG_CAMM == 0
	DWORD threadID;
	if (CreateThread(NULL, 0, Comm_Process, NULL, 0, &threadID) == NULL)
	{
		printf("创建线程失败!");
		Exit_ProcessFlag = false;
	}
#endif
	const int wxScreen[] = { 1920, 1080 };//单块屏幕大小

	const int wxFrameWLP[] = { 1920, 1080, 30 };// 摄像头像素大小以及帧率

	char c;

	Mat frame(wxScreen[1], wxScreen[0] * 2, CV_8UC3, Scalar(0, 0, 0));
	if (frame.empty()){
		return -1;
	}


	Rect rect0((int)((wxScreen[0] - wxFrameWLP[0]) / 2), (int)((wxScreen[1] - wxFrameWLP[1]) / 2), wxFrameWLP[0], wxFrameWLP[1]);
	Rect rect1((int)(wxScreen[0] + (wxScreen[0] - wxFrameWLP[0]) / 2), (int)((wxScreen[1] - wxFrameWLP[1]) / 2), wxFrameWLP[0], wxFrameWLP[1]);

	Mat CopyFrame0 = frame(rect0);
	Mat CopyFrame1 = frame(rect1);

	VideoCapture cap0 = VideoCapture(0);
	if (!cap0.isOpened()){
		cvDestroyAllWindows();
		return -2;
	}
	VideoCapture cap1 = VideoCapture(1);
	if (!cap1.isOpened()){
		cap0.release();
		cvDestroyAllWindows();
		return -3;
	}

	cap0.set(CV_CAP_PROP_FRAME_WIDTH, wxFrameWLP[0]);//设置摄像头采集图像分辨率
	cap0.set(CV_CAP_PROP_FRAME_HEIGHT, wxFrameWLP[1]);
	cap0.set(CV_CAP_PROP_FPS, wxFrameWLP[2]);
	cap1.set(CV_CAP_PROP_FRAME_WIDTH, wxFrameWLP[0]);//设置摄像头采集图像分辨率
	cap1.set(CV_CAP_PROP_FRAME_HEIGHT, wxFrameWLP[1]);
	cap1.set(CV_CAP_PROP_FPS, wxFrameWLP[2]);

	namedWindow("frame", CV_WINDOW_NORMAL);//CV_WINDOW_NORMAL);
	moveWindow("frame", wxScreen[0], 0);
	setWindowProperty("frame", CV_WINDOW_FULLSCREEN, CV_WINDOW_NORMAL);

	Mat frame0, frame1;
	cap0 >> frame0;
	cap1 >> frame1;

	printf("%d %d %d %d %lf\n", frame0.cols, frame0.rows, frame0.depth(), frame0.channels(), cap0.get(CV_CAP_PROP_FPS));
	printf("%d %d %d %d %lf\n", frame1.cols, frame1.rows, frame1.depth(), frame1.channels(), cap1.get(CV_CAP_PROP_FPS));

	bool Switch_Cam = true;
	uint8_t Get_CammFrameFailed[2];
	while (Exit_ProcessFlag)// && !frame0.empty() && !frame1.empty())
	{
		if (frame0.empty()){
			if (++Get_CammFrameFailed[0] == 5){
				break;
			}
		}
		else if (Get_CammFrameFailed[0] != 0){
			Get_CammFrameFailed[0] = 0;
		}

		if (frame1.empty()){
			if (++Get_CammFrameFailed[1] == 5){
				break;
			}
		}
		else if (Get_CammFrameFailed[1] != 0){
			Get_CammFrameFailed[1] = 0;
		}

		frame0.convertTo(CopyFrame0, CopyFrame0.type(), 1, 0);
		frame1.convertTo(CopyFrame1, CopyFrame1.type(), 1, 0);
		imshow("frame", frame);
		c = waitKey(1000 / wxFrameWLP[2]);
		if ((c == 27) || (c == 'q'))
		{
			break;
		}
		if (Switch_Cam){
			cap0 >> frame0;
		}else{
			cap1 >> frame1;
		}
		Switch_Cam = !Switch_Cam;
	}

	cap0.release();
	cap1.release();
	frame.release();
	frame0.release();
	frame1.release();
	cvDestroyAllWindows();

	Exit_ProcessFlag = false;
	Sleep(1000);
	return 0;
}
#endif

//int main(void){
//	Mat image = imread("dh.jpg");
//	Rect rect1(10, 10, 50, 50);
//	Rect rect2(50, 50, 50, 50);
//	Mat roi1 = image(rect1); // copy the region rect1 from the image to roi1
//
//	Mat roi2 = image(rect2); // copy the region rect1 from the image to roi1
//
//	imshow("1", roi1);
//	imshow("2", roi2);
//	roi2.convertTo(roi1, roi1.type(), 1, 0);
//
//	imshow("0", image);
//	imshow("3", roi1);
//	imshow("4", roi2);
//	waitKey(0);
//	image.release();
//	cvDestroyAllWindows();
//}