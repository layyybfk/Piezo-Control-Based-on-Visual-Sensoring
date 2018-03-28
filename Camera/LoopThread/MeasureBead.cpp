/********************************************************************************/
/*说明：对单个微球的测量的实现，包括测量XYZ位置，以及建立Z向校正模型的基本函数
/*日期：2016，11，9
/*作者：
/*版本： 注意，后期应当在主程序中将statX Y 两个变量加上move即压电平台的位移量，这样更有利于测量
startX 在CposiControlDlg中应该用 extern 变量更好
/********************************************************************************/
#include "StdAfx.h"
#include "MeasureBead.h"
#include "math.h"
#include "LoopThreadDlg.h"
#include "iostream"
#include "fstream"
#include "conio.h" //控制台输出 &LTD
#include <algorithm>

using namespace std;

#define SIZE	  400				//最大的框选范围
#define CAMERAWIDTH 1282
#define MODELSIZE (SIZE/2+1)
#define MAXMODELSIZE 901

double PosiX = 0;					//输出的XY的位置
double PosiY = 0;
double PosiZ = 0;
double PosiX2 = 0;
double PosiY2 = 0;
double PosiZ2 = 0;
double PosiXX = 0;
double PosiYY = 0;
double PosiZZ = 0;
double PosiXX2 = 0;
double PosiYY2 = 0;
double PosiZZ2 = 0;
int cishu = 1;
double MotionX[2 * SIZE - 1];
double MotionY[2 * SIZE - 1];



MeasureBead::MeasureBead(void)
{
	Size = SIZE;
	CameraWidth = CAMERAWIDTH;
	//linesum = new double[SIZE / 2 + 1];
	//linenum = new double[SIZE / 2 + 1];

	Array = new uint8_t [SIZE*SIZE];
	Gauss = new uint8_t [SIZE*SIZE];
	Array_ = new uint8_t[SIZE*SIZE];
	Gauss_ = new uint8_t[SIZE*SIZE];

	UArrayRow = new int [SIZE];
	UArrayCol = new int [SIZE];
	UArrayRow_ = new int[SIZE];
	UArrayCol_ = new int[SIZE];

	Score = new double[SIZE / 2 + 1];
	OriginRoi = new uint8_t [SIZE*SIZE];

	//gaussian filter
	b = new float[4];
	Computecoefs(&B, b, 1);
}
MeasureBead::~MeasureBead(void)
{
	//delete[]linesum;
	//delete[]linenum;
	delete[]Array;
	delete[]Gauss;
	delete[]Array_;
	delete[]Gauss_;
	delete[]UArrayRow;
	delete[]UArrayCol;
	delete[]UArrayRow_;
	delete[]UArrayCol_;
	delete[]Score;
	delete[]OriginRoi;
}

/*MeasureXY：功能为求出XY的位置，并追踪实时显示XY的位置（微球中心）*/
void MeasureBead::MeasureXY(uint8_t ImageArray[], int *StartX, int *StartY, double *PosiX, double *PosiY)
{
	double OffsetX = 0, OffsetY = 0;
	double TempPosX = 0, TempPosY = 0;

	double max_value = 0.0;
	double min_value = 255.0;
	double temp = 0.0;
	int threshold = 0;

	//======提取图像程序===================================================
	//ofstream outfile1;
	//outfile1.open("C:/Users/layyybfk/Desktop/data/1.dat");
	for (int i = 0; i < Size; i++)
	{
		for (int j = 0; j < Size; j++)
		{
			Array[i*Size + j] = 255.0 - ImageArray[(i + *StartY) * CameraWidth + j + *StartX];
			//outfile1 << (short)Array[i*Size + j] << " ";
		}
		//outfile1 << endl;
		UArrayRow[i] = 0;
		UArrayCol[i] = 0;
	}
	//outfile1.close();
	//======快速高斯平滑测试==================================================================
	//======阈值处理思想算法==================================================================
	threshold = (Array[1] + Array[2] + Array[3] + Array[4]) / 4 + 25;
	//threshold = mean() + 15;
	ofstream outfile2;
	outfile2.open("C:/Users/layyybfk/Desktop/data/2.dat");
	for (int i = 0; i < Size; i++)
	{
		for (int j = 0; j < Size; j++)
		{
			if (Array[i*Size + j] < threshold && (i - (Size / 2)) * (i - (Size / 2)) + (j - (Size / 2)) * (j - (Size / 2)) > 35 * 35)
				Array[i*Size + j] = 0;
			outfile2 << (short)Array[i*Size + j] << " ";
		}
		outfile2 << endl;
	}
	outfile2.close();
	//================高斯平滑==============================================================================================
	//========灰度变换(归一化映射)==========================================================================================
	for (int i = 0; i < Size; i++)
	{
		for (int j = 0; j < Size; j++)
		{
			if (Array[i*Size + j] > max_value)
				max_value = Array[i*Size + j];
			if (Array[i*Size + j] < min_value && Array[i*Size + j] != 0)	//不能把0算成最小值
				min_value = Array[i*Size + j];
		}
	}
	ofstream outfile3;
	outfile3.open("C:/Users/layyybfk/Desktop/data/3.dat");
	for (int i = 0; i < Size; i++)
	{
		for (int j = 0; j < Size; j++)
		{
			if (Array[i*Size + j] != 0)
				Array[i*Size + j] = 255.0 * (((double)Array[i*Size + j] - min_value) / (max_value - min_value));
			outfile3 << (short)Array[i*Size + j] << " ";
		}
		outfile3 << endl;
	}
	outfile3.close();

	GaussianFilter(Array, Gauss, Size, Size);
	memcpy(Array, Gauss, Size*Size * sizeof(uint8_t));  //复制像素  

	ofstream outfile4;
	outfile4.open("C:/Users/layyybfk/Desktop/data/4.dat");
	for (int i = 0; i < Size; i++)
	{
		for (int j = 0; j < Size; j++)
		{
			outfile4 << (short)Array[i*Size + j] << " ";
		}
		outfile4 << endl;
	}
	outfile4.close();

	////======计算中心===============================================================================================================
	for (int i = 0; i < Size; i++)
	{
		for (int j = 0; j < Size; j++)
		{
			UArrayRow[i] += Array[i*Size + j];
			UArrayCol[j] += Array[i*Size + j];
		}
	}
	for (int kt = 0; kt<2 * Size - 1; kt++)
	{
		MotionY[kt] = 0;
		MotionX[kt] = 0;
		for (int jt = max(0, kt + 1 - Size); jt <= min(kt, Size - 1); jt++)
		{
			MotionY[kt] += UArrayRow[jt] * UArrayRow[kt - jt];
			MotionX[kt] += UArrayCol[jt] * UArrayCol[kt - jt];
		}
	}
	ofstream outfile_array;
	outfile_array.open("C:/Users/layyybfk/Desktop/data/array.dat");
	for (int i = 0; i < 2 * Size; i++)
		outfile_array << MotionX[i] << " ";
	outfile_array.close();

	FindMax(MotionX, &OffsetX, Size);
	FindMax(MotionY, &OffsetY, Size);

	//_cprintf("Position of the bead is: %f\n\n", OffsetX);
	//_cprintf("Position of the bead is: %f\n\n", OffsetY);

	TempPosX = (double)*StartX + OffsetX;
	TempPosY = (double)*StartY + OffsetY;
	*StartX = (int)(TempPosX + 0.5);
	*StartY = (int)(TempPosY + 0.5);
	*PosiX = TempPosX + SIZE / 2;
	*PosiY = TempPosY + SIZE / 2;
}

void MeasureBead::MeasureXY_(uint8_t ImageArray[], int *StartX, int *StartY, double *PosiX, double *PosiY)
{
	double OffsetX = 0, OffsetY = 0;
	double TempPosX = 0, TempPosY = 0;

	double max_value = 0.0;
	double min_value = 255.0;
	double temp = 0.0;
	int threshold = 0;

	//======提取图像程序===================================================
	for (int i = 0; i < Size; i++)
	{
		for (int j = 0; j < Size; j++)
		{
			Array_[i*Size + j] = -ImageArray[(i + *StartY) * 1282 + j + *StartX];
		}
		//outfile << endl;
		UArrayRow_[i] = 0;
		UArrayCol_[i] = 0;
	}
	//======快速高斯平滑测试==================================================================
	//======阈值处理思想算法==================================================================
	threshold = (Array_[1] + Array_[2] + Array_[3] + Array_[4]) / 4 + 25;

	for (int i = 0; i < Size; i++)
	{
		for (int j = 0; j < Size; j++)
		{
			if (Array_[i*Size + j] < threshold && (i - (Size / 2)) * (i - (Size / 2)) + (j - (Size / 2)) * (j - (Size / 2)) > 35 * 35)
				Array_[i*Size + j] = 0;
		}
	}
	//================高斯平滑==============================================================================================
	//========灰度变换(归一化映射)==========================================================================================
	for (int i = 0; i < Size; i++)
	{
		for (int j = 0; j < Size; j++)
		{
			if (Array_[i*Size + j] > max_value)
				max_value = Array_[i*Size + j];
			if (Array_[i*Size + j] < min_value && Array_[i*Size + j] != 0)	//不能把0算成最小值
				min_value = Array_[i*Size + j];
		}
	}

	for (int i = 0; i < Size; i++)
	{
		for (int j = 0; j < Size; j++)
		{
			if (Array_[i*Size + j] != 0)
				Array_[i*Size + j] = 255.0 * (((double)Array_[i*Size + j] - min_value) / (max_value - min_value));
		}
	}

	GaussianFilter(Array_, Gauss_, Size, Size);
	memcpy(Array_, Gauss_, Size*Size * sizeof(uint8_t));  //复制像素  

	//======计算中心===============================================================================================================
	//分别将每一行，每一列的灰度值相加，然后求平均
	for (int i = 0; i < Size; i++)
	{
		for (int j = 0; j < Size; j++)
		{
			UArrayRow_[i] += Array_[i*Size + j];
			UArrayCol_[j] += Array_[i*Size + j];
		}
	}
	FindCentroid(UArrayRow_, &OffsetY, Size);
	FindCentroid(UArrayCol_, &OffsetX, Size);

	TempPosX = (double)*StartX + OffsetX;
	TempPosY = (double)*StartY + OffsetY;
	*StartX = (int)(TempPosX + 0.5);
	*StartY = (int)(TempPosY + 0.5);
	*PosiX = TempPosX + SIZE / 2;
	*PosiY = TempPosY + SIZE / 2;
}

void MeasureBead::FindMax(double *Motion, double *Offset, int Msize)
{
	int kt;
	double temp; //Maxima
	int tempk;
	int dmm, dma, dmb; //Gaussian Fit
	double Fm, Fma, Fmb;
	double D;

	temp = Motion[0];
	tempk = 0;
	for (kt = 1; kt < 2 * Msize - 1; kt++)
	{
		if (Motion[kt]>temp)
		{
			temp = Motion[kt];
			tempk = kt;
		}
	}

	//3 point Gaussian fit
	dmm = tempk;
	dma = tempk - 1;
	dmb = tempk + 1;

	Fm = Motion[dmm];
	Fma = Motion[dma];
	Fmb = Motion[dmb];
	D = (log(Fm) - log(Fmb))*(dmm*dmm - dma*dma) / 2 / (2 * log(Fm) - log(Fma) - log(Fmb))
		-
		(log(Fm) - log(Fma))*(dmm*dmm - dmb*dmb) / 2 / (2 * log(Fm) - log(Fma) - log(Fmb));
	*Offset = 0.5*(D - Msize);
}

void MeasureBead::FindCentroid(int *Array, double *Offset, int Size)
{
	int i;
	double sum;
	double sumi;
	sum = 0;
	sumi = 0;
	for (i = 0; i<Size; i++)
	{
		sum = sum + Array[i];
		sumi = sumi + Array[i] * i;
	}
	*Offset = sumi / sum - (Size - 1.0) / 2.0;
}


/*调用函数找出微球的位移量，并将中心到原点的偏移量存入OffsetX,OffsetY，最后生成该幅图像的径向矢量*/
//调用这个功能可以建立校正模型，也可以用来测量Z位置。//line为该图像的径向矢量
void MeasureBead::CalMod(unsigned char *ImageArray, int *StartX, int *StartY, double *line, double *PosiX, double *PosiY)
{
	double OffsetX = 0, OffsetY = 0;
	double TempPosX = 0, TempPosY = 0;
	double max_value = 0;
	double min_value = 255;
	double temp = 0.0;
	int threshold = 0;

	//找到阈值部分
	threshold = (-ImageArray[(0 + *StartY) * CameraWidth + 0 + *StartX] -
		ImageArray[(0 + *StartY) * CameraWidth + 1 + *StartX] -
		ImageArray[(0 + *StartY) * CameraWidth + 2 + *StartX] -
		ImageArray[(0 + *StartY) * CameraWidth + 3 + *StartX]) / 4 + 15;

	//将图像导入到array[size^2]中,绘制矩形框的时候改变了startx的值，为当时矩形框的左上角
	for (int i = 0; i < Size; i++)
	{
		for (int j = 0; j < Size; j++)
		{
			if (-ImageArray[(i + *StartY) * CameraWidth + j + *StartX] > threshold || (i - (Size / 2))*(i - (Size / 2)) + (j - (Size / 2))*(j - (Size / 2)) < (70 / 2)*(70 / 2))
				Array[i*Size + j] = -ImageArray[(i + *StartY) * CameraWidth + j + *StartX];
			else
				Array[i*Size + j] = 0;
			if (Array[i*Size + j] > max_value)
				max_value = Array[i*Size + j];
			if (Array[i*Size + j] < min_value && Array[i*Size + j] != 0)	//不能把0算成最小值
				min_value = Array[i*Size + j];
			OriginRoi[i*Size + j] = ImageArray[(i + *StartY) * CameraWidth + j + *StartX];
		}
		UArrayRow[i] = 0;
		UArrayCol[i] = 0;
	}

	//高斯平滑
	//	GaussianFilter(Array,Gauss,Size,Size);
	//	memcpy ( Array, Gauss, Size*Size*sizeof(unsigned char) );  //复制像素  
	GaussianFilter(OriginRoi, Gauss, Size, Size);
	memcpy(OriginRoi, Gauss, Size*Size * sizeof(unsigned char));

	//归一化校正
	for (int i = 0; i < Size; i++)
	{
		for (int j = 0; j < Size; j++)
		{
			if (Array[i*Size + j] != 0)
			{
				temp = ((double)Array[i*Size + j] - min_value) / (max_value - min_value);
				Array[i*Size + j] = 255.0 *  temp;
			}
		}
	}

	//分别将每一行，每一列的灰度值相加，然后求平均
	for (int i = 0; i < Size; i++)
	{
		for (int j = 0; j < Size; j++)
		{
			UArrayRow[i] += Array[i*Size + j];
			UArrayCol[j] += Array[i*Size + j];
		}
	}
	FindCentroid(UArrayRow, &OffsetY, Size);
	FindCentroid(UArrayCol, &OffsetX, Size);
	TempPosX = (double)*StartX + OffsetX;
	TempPosY = (double)*StartY + OffsetY;
	*StartX = (int)(TempPosX + 0.5);
	*StartY = (int)(TempPosY + 0.5);
	////	}while((fabs(OffsetY)>0.5)||(fabs(OffsetX)>0.5));//在中心到原点的距离小于1时候，则可以认为找到中心结束
	*PosiX = TempPosX + SIZE / 2;
	*PosiY = TempPosY + SIZE / 2;
	//*PosiX = TempPosX;					//此时的微球中心位置	//函数的输出值
	//*PosiY = TempPosY;

	double Xpos = OffsetX + SIZE / 2;					//此时的微球中心位置,这时候为相对位置
	double Ypos = OffsetY + SIZE / 2;

	double	linesum[SIZE / 2 + 1] = { 0.0 };	//126，分别为126个同心圆环的灰度总值以及总像素点个数
	double	linenum[SIZE / 2 + 1] = { 0.0 };
	double	d;					//distance
	int		ds;
	double	coef, ccoef;
	double sum = 0;

	//转化为径向矢量
	for (int i = 0; i < SIZE; i++)		//250
	{
		for (int j = 0; j < SIZE; j++)
		{
			//d为当前点对于中心点Xpos,Ypos的距离，ds向下取整数
			d = sqrt((double(i) - Xpos)*(double(i) - Xpos) + (double(j) - Ypos)*(double(j) - Ypos));
			ds = (int)floor(d);
			if (d < SIZE / 2)			//没有超出图像范围
			{
				ccoef = d - ds;			//分别为内环外环所占比例,间距默认为1个像素
				coef = 1 - ccoef;
				linesum[ds] = linesum[ds] + OriginRoi[j*SIZE + i] * coef;		//image已经转换为一维数组【250*250】
				linesum[ds + 1] = linesum[ds + 1] + OriginRoi[j*SIZE + i] * ccoef;
				//linesum[ds]=linesum[ds]+Array[jp*SIZE+ip]*coef;		//image已经转换为一维数组【250*250】
				//linesum[ds+1]=linesum[ds+1]+Array[jp*SIZE+ip]*ccoef;
				linenum[ds] = linenum[ds] + coef;
				linenum[ds + 1] = linenum[ds + 1] + ccoef;
			}
		}
	}
	for (int i = 0; i < SIZE / 2 + 1; i++)
	{
		line[i] = linesum[i] / linenum[i];			//即求出每个圆环的平均灰度值。
	}
	for (int i = 0; i < SIZE / 2 + 1; i++)
	{
		sum = sum + line[i] / (SIZE / 2 + 1);
	}
	for (int i = 0; i<SIZE / 2 + 1; i++)
	{
		line[i] = line[i] - sum;
	}
}

//测量Z位置
void MeasureBead::MeasureZ(double *ModelMatrix, double *line, double StartPosition, double EndPosition, double StepSize, double *PosiZ)
{
	int			kmin;
	long double Score[MaxModelSize] = { 0 };			//901张图片最多
	long double temp;
	long double Tempmin;
	double 		peakZ;
	int			ModelNum = int((EndPosition - StartPosition) / StepSize + 1);

	//功能将校正模型里面所有的模型与line比较，score[iz],记录和校正模型每一幅图（不选最后一幅）里面的差,找到最小值，然后输出给Tempmin，kmin
	for (int iz = 0; iz<ModelNum; iz++)			//level =1目前，校正模型总共有modelnum个,可以定义level
	{
		for (int jz = 0; jz<MODELSIZE; jz++)				//126,
		{
			temp = line[jz] - ModelMatrix[iz*MODELSIZE + jz];	// ModelMatrix的大小为901*126 
			Score[iz] += (temp*temp);
		}
		//输出tempmin
		if (iz == 0)
		{
			Tempmin = Score[0];
			kmin = 0;
		}
		else
		{
			if (Score[iz]<Tempmin)
			{
				Tempmin = Score[iz];
				kmin = iz;			//表示第几个图片的校正模型
			}
		}
	}

	//然后需要找到m-1,m+1张的数据，然后传过去比较
	if (kmin != 0 && kmin != ModelNum)
	{
		int dmm = kmin;
		int dma = kmin - 1;
		int dmb = kmin + 1;

		double Fm = Score[dmm];
		double Fma = Score[dma];
		double Fmb = Score[dmb];

		peakZ = (Fm - Fmb)*(dmm*dmm - dma*dma) / 2 / (2 * Fm - Fma - Fmb) -
			(Fm - Fma)*(dmm*dmm - dmb*dmb) / 2 / (2 * Fm - Fma - Fmb);

	}
	else if (kmin = 0)
	{
		//说明图像在0-1之间，用其他的插值方法	
		peakZ = 0;
	}
	else
	{
		peakZ = ModelNum;
	}
	*PosiZ = (StartPosition + peakZ*StepSize); //Position Z  	
}

//普通高斯滤波
void MeasureBead::GaussianFilter(uint8_t* Src, uint8_t* Dest, int width, int height)
{
	int templates[9] = { 1, 1, 1 ,
		1, 1, 1,
		1, 1, 1, };        //滤波器模板  
	memcpy(Dest, Src, width*height * sizeof(uint8_t));  //复制像素  
	for (int j = 2; j<height - 2; j++)  //边缘不处理  
	{
		for (int i = 2; i<width - 2; i++)
		{
			int sum = 0;
			int index = 0;
			for (int m = j - 1; m<j + 2; m++)
			{
				for (int n = i - 1; n<i + 2; n++)
				{
					sum += Src[m*width + n] * templates[index++];
				}
			}
			sum /= 9;
			if (sum > 255)
				sum = 255;
			Dest[j*width + i] = sum;
		}
	}
}


//================FAST Gaussian Filter based on IIR============================================================
//滤波参数计算  
void MeasureBead::Computecoefs(float *B, float *b, float sigma)
{
	float q, q2, q3;
	if (sigma >= 2.5)
	{
		q = 0.98711 * sigma - 0.96330;
	}
	else if ((sigma >= 0.5) && (sigma < 2.5))
	{
		q = 3.97156 - 4.14554 * (float)sqrt((float)1 - 0.26891 * sigma);
	}
	else
	{
		q = 0.1147705018520355224609375;
	}
	q2 = q * q;
	q3 = q * q2;
	b[0] = 1 / (1.57825 + (2.44413*q) + (1.4281 *q2) + (0.422205*q3));
	b[1] = ((2.44413*q) + (2.85619*q2) + (1.26661 *q3)) *b[0];
	b[2] = (-((1.4281*q2) + (1.26661 *q3)))*b[0];
	b[3] = ((0.422205*q3)) *b[0];
	*B = 1.0 - (b[1] + b[2] + b[3]);
}

//IIR型高斯滤波    
void MeasureBead::FastGaussianFilter(unsigned char* Src, unsigned char* Dest, float B, float *b)
{
	float *Buff, *BPointer, *w1, *w2;

	Buff = (float*)malloc(sizeof(float)*SIZE*SIZE);//分配缓存空间  
	if (Buff == NULL)
	{
		AfxMessageBox(_T("生成失败，完犊子！"));
		return;
	}
	//拷贝原图至缓存Buff  
	memcpy(Buff, Src, SIZE*SIZE * sizeof(unsigned char));  //复制像素  

														   //横向滤波  
	BPointer = Buff;  //指针指向buff首位置
					  //分配
	w1 = (float*)malloc(sizeof(float)*(SIZE + 4));
	w2 = (float*)malloc(sizeof(float)*(SIZE + 4));
	for (int Y = 0; Y<SIZE; Y++)
	{
		//前向滤波  
		w1[0] = w1[1] = w1[2] = BPointer[0];
		for (int n = 3, i = 0; i<SIZE; n++, i++)
		{
			w1[n] = B*BPointer[i] + (b[1] * w1[n - 1] + b[2] * w1[n - 2] + b[3] * w1[n - 3]);
		}
		//后向滤波  
		w2[SIZE] = w2[SIZE + 1] = w2[SIZE + 2] = w1[SIZE + 2];
		for (int n = SIZE - 1; n >= 0; n--)
		{
			BPointer[n] = w2[n] = B*w1[n + 3] + (b[1] * w2[n + 1] + b[2] * w2[n + 2] + b[3] * w2[n + 3]);
		}
		BPointer += SIZE;
	}

	//纵向滤波  
	BPointer = Buff;
	w1 = (float*)realloc(w1, sizeof(float)*(SIZE + 3));
	w2 = (float*)realloc(w2, sizeof(float)*(SIZE + 3));
	for (int X = 0; X<SIZE; X++)
	{
		//前向滤波  
		w1[0] = w1[1] = w1[2] = BPointer[0];
		for (int n = 3, i = 0; i<SIZE; n++, i++)
		{
			w1[n] = B*BPointer[i*SIZE] + (b[1] * w1[n - 1] + b[2] * w1[n - 2] + b[3] * w1[n - 3]);
		}
		//后向滤波  
		w2[SIZE] = w2[SIZE + 1] = w2[SIZE + 2] = w1[SIZE + 2];
		for (int n = SIZE - 1; n >= 0; n--)
		{
			BPointer[n*SIZE] = w2[n] = B*w1[n + 3] + (b[1] * w2[n + 1] + b[2] * w2[n + 2] + b[3] * w2[n + 3]);
		}
		BPointer++;
	}

	//拷贝缓存Buff到结果 
	memcpy(Dest, Buff, SIZE*SIZE * sizeof(unsigned char));  //复制像素  
	free(w1);
	free(w2);
}

double MeasureBead::KalmanFilter(double ResrcData, double ControlValue, double ProcessNiose_Q, double MeasureNoise_R, double InitialPrediction, long FrequencyDisp, int Channel)
{
	//确定第一次的控制量,和测量值相同，减少迭代次数
	switch (Channel)
	{
	case 0:
	{
		if (FrequencyDisp == 1)
		{
			X_last = ResrcData;
			Xp_last = InitialPrediction;
			XControlValue_last = ControlValue;
		}

		XControlAdd = (ControlValue - XControlValue_last) * 12;
		//XControlAdd = 0;
		//上一次系统最优化值赋给一个中间变量
		X_mid = X_last + XControlAdd;		//x_last=x(k-1|k-1),x_mid=x(k|k-1)
		Xp_mid = Xp_last + ProcessNiose_Q;	//p_mid=p(k|k-1),p_last=p(k-1|k-1),Q=噪声

		Xkg = Xp_mid / (Xp_mid + MeasureNoise_R);	//kg为kalman filter，R为噪声

		X_now = X_mid + Xkg*(ResrcData - X_mid);	//估计出的最优值 
		Xp_now = (1 - Xkg)*Xp_mid;//最优值对应的covariance  

								  //更新系统最优化值和协方差
		Xp_last = Xp_now; //更新covariance值
		X_last = X_now; //更新系统状态值
		XControlValue_last = ControlValue; //更新控制值
		return X_now;
	}
	case 1:
	{
		if (FrequencyDisp == 1)
		{
			Y_last = ResrcData;
			Yp_last = InitialPrediction;
			YControlValue_last = ControlValue;
		}

		YControlAdd = YControlValue - YControlValue_last;
		//上一次系统最优化值赋给一个中间变量
		Y_mid = Y_last + YControlAdd;		//Y_last=Y(k-1|k-1),Y_mid=Y(k|k-1)
		Yp_mid = Yp_last + ProcessNiose_Q;	//p_mid=p(k|k-1),p_last=p(k-1|k-1),Q=噪声

		Ykg = Yp_mid / (Yp_mid + MeasureNoise_R);	//kg为kalman filter，R为噪声

		Y_now = Y_mid + Ykg*(ResrcData - Y_mid);	//估计出的最优值 
		Yp_now = (1 - Ykg)*Yp_mid;//最优值对应的covariance  

								  //更新系统最优化值和协方差
		Yp_last = Yp_now; //更新covariance值
		Y_last = Y_now; //更新系统状态值
		YControlValue_last = ControlValue; //更新控制值
		return Y_now;
	}
	case 2:
	{
		if (FrequencyDisp == 1)
		{
			Z_last = ResrcData;
			Zp_last = InitialPrediction;
			ZControlValue_last = ControlValue;
		}

		ZControlAdd = ZControlValue - ZControlValue_last;
		//上一次系统最优化值赋给一个中间变量
		Z_mid = Z_last + ZControlAdd;		//Z_last=Z(k-1|k-1),Z_mid=Z(k|k-1)
		Zp_mid = Zp_last + ProcessNiose_Q;	//p_mid=p(k|k-1),p_last=p(k-1|k-1),Q=噪声

		Zkg = Zp_mid / (Zp_mid + MeasureNoise_R);	//kg为kalman filter，R为噪声

		Z_now = Z_mid + Zkg*(ResrcData - Z_mid);	//估计出的最优值 
		Zp_now = (1 - Zkg)*Zp_mid;//最优值对应的covariance  

								  //更新系统最优化值和协方差
		Zp_last = Zp_now; //更新covariance值
		Z_last = Z_now; //更新系统状态值
		ZControlValue_last = ControlValue; //更新控制值
		return Z_now;
	}
	}

}
