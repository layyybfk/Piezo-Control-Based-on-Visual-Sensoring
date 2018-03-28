/********************************************************************************/
/*说明：整个球的测量程序，包含建立校正模型，测量XYZ三个维度的位置的函数
/*日期：2016，9，21
/*作者：
/*
/********************************************************************************/
#include <stdint.h>
#pragma once
#define MSIZE	200			//最大校正模型长度
#define MaxModelSize 901	//最大

extern double PosiX;
extern double PosiY;
extern double PosiZ;
extern double PosiX2;
extern double PosiY2;
extern double PosiZ2;
extern double PosiXX;
extern double PosiYY;
extern double PosiZZ;
extern double PosiXX2;
extern double PosiYY2;
extern double PosiZZ2;
extern int cishu;

class MeasureBead
{
public:
	MeasureBead(void);
	~MeasureBead(void);

	void MeasureXY(uint8_t ImageArray[], int* StartX, int* StartY, double *PosiX, double *PosiY);//追踪 XY 坐标值
	void MeasureXY_(uint8_t ImageArray[], int* StartX, int* StartY, double *PosiX, double *PosiY);//追踪 XY 坐标值
	void CalMod(unsigned char *ImageArray, int *StartX, int *StartY, double *line, double *PosiX, double *PosiY);				//总函数,测量XY并且得到
	void MeasureZ(double *ModelMatrix, double *line, double StartPosition, double EndPosition, double StepSize, double *PosiZ);	//测量Z坐标值
	void FindMax(double *Motion, double *Offset, int Msize);
	void FindCentroid(int *Array, double *Offset, int Msize);
	void GaussianFilter(uint8_t* Src, uint8_t* Dest, int width, int height);

	//IIR快速高斯算法
	void Computecoefs(float *B, float *b, float sigma);
	void FastGaussianFilter(unsigned char* Src, unsigned char* Dest, float B, float *b);

	//卡尔曼滤波算法
	double KalmanFilter(double ResrcData, double ControlValue, double ProcessNiose_Q, double MeasureNoise_R, double InitialPrediction, long FrequencyDisp, int Channel);

public:
	int Size;
	//动态分配内存相关
	//double* linesum;
	//double* linenum;

	uint8_t * Array;
	uint8_t * Gauss;
	uint8_t * Array_;
	uint8_t * Gauss_;

	int * UArrayRow;
	int * UArrayCol;
	int * UArrayRow_;
	int * UArrayCol_;

	int CameraWidth;
	int CameraHeight;
	double * Score;
	uint8_t * OriginRoi;

	//Gaussian filter相关
	float B;
	float *b;

	//Kalman filter相关,分离定义XYZ三个轴
	double X_last, X_mid, X_now;
	double Y_last, Y_mid, Y_now;
	double Z_last, Z_mid, Z_now;

	double Xp_last, Xp_mid, Xp_now;
	double Yp_last, Yp_mid, Yp_now;
	double Zp_last, Zp_mid, Zp_now;

	double XControlValue_last, XControlAdd, XControlValue;
	double YControlValue_last, YControlAdd, YControlValue;
	double ZControlValue_last, ZControlAdd, ZControlValue;

	double Xkg;
	double Ykg;
	double Zkg;
};








