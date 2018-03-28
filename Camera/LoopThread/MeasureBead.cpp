/********************************************************************************/
/*˵�����Ե���΢��Ĳ�����ʵ�֣���������XYZλ�ã��Լ�����Z��У��ģ�͵Ļ�������
/*���ڣ�2016��11��9
/*���ߣ�
/*�汾�� ע�⣬����Ӧ�����������н�statX Y ������������move��ѹ��ƽ̨��λ�����������������ڲ���
startX ��CposiControlDlg��Ӧ���� extern ��������
/********************************************************************************/
#include "StdAfx.h"
#include "MeasureBead.h"
#include "math.h"
#include "LoopThreadDlg.h"
#include "iostream"
#include "fstream"
#include "conio.h" //����̨��� &LTD
#include <algorithm>

using namespace std;

#define SIZE	  400				//���Ŀ�ѡ��Χ
#define CAMERAWIDTH 1282
#define MODELSIZE (SIZE/2+1)
#define MAXMODELSIZE 901

double PosiX = 0;					//�����XY��λ��
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

/*MeasureXY������Ϊ���XY��λ�ã���׷��ʵʱ��ʾXY��λ�ã�΢�����ģ�*/
void MeasureBead::MeasureXY(uint8_t ImageArray[], int *StartX, int *StartY, double *PosiX, double *PosiY)
{
	double OffsetX = 0, OffsetY = 0;
	double TempPosX = 0, TempPosY = 0;

	double max_value = 0.0;
	double min_value = 255.0;
	double temp = 0.0;
	int threshold = 0;

	//======��ȡͼ�����===================================================
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
	//======���ٸ�˹ƽ������==================================================================
	//======��ֵ����˼���㷨==================================================================
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
	//================��˹ƽ��==============================================================================================
	//========�Ҷȱ任(��һ��ӳ��)==========================================================================================
	for (int i = 0; i < Size; i++)
	{
		for (int j = 0; j < Size; j++)
		{
			if (Array[i*Size + j] > max_value)
				max_value = Array[i*Size + j];
			if (Array[i*Size + j] < min_value && Array[i*Size + j] != 0)	//���ܰ�0�����Сֵ
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
	memcpy(Array, Gauss, Size*Size * sizeof(uint8_t));  //��������  

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

	////======��������===============================================================================================================
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

	//======��ȡͼ�����===================================================
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
	//======���ٸ�˹ƽ������==================================================================
	//======��ֵ����˼���㷨==================================================================
	threshold = (Array_[1] + Array_[2] + Array_[3] + Array_[4]) / 4 + 25;

	for (int i = 0; i < Size; i++)
	{
		for (int j = 0; j < Size; j++)
		{
			if (Array_[i*Size + j] < threshold && (i - (Size / 2)) * (i - (Size / 2)) + (j - (Size / 2)) * (j - (Size / 2)) > 35 * 35)
				Array_[i*Size + j] = 0;
		}
	}
	//================��˹ƽ��==============================================================================================
	//========�Ҷȱ任(��һ��ӳ��)==========================================================================================
	for (int i = 0; i < Size; i++)
	{
		for (int j = 0; j < Size; j++)
		{
			if (Array_[i*Size + j] > max_value)
				max_value = Array_[i*Size + j];
			if (Array_[i*Size + j] < min_value && Array_[i*Size + j] != 0)	//���ܰ�0�����Сֵ
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
	memcpy(Array_, Gauss_, Size*Size * sizeof(uint8_t));  //��������  

	//======��������===============================================================================================================
	//�ֱ�ÿһ�У�ÿһ�еĻҶ�ֵ��ӣ�Ȼ����ƽ��
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


/*���ú����ҳ�΢���λ�������������ĵ�ԭ���ƫ��������OffsetX,OffsetY��������ɸ÷�ͼ��ľ���ʸ��*/
//����������ܿ��Խ���У��ģ�ͣ�Ҳ������������Zλ�á�//lineΪ��ͼ��ľ���ʸ��
void MeasureBead::CalMod(unsigned char *ImageArray, int *StartX, int *StartY, double *line, double *PosiX, double *PosiY)
{
	double OffsetX = 0, OffsetY = 0;
	double TempPosX = 0, TempPosY = 0;
	double max_value = 0;
	double min_value = 255;
	double temp = 0.0;
	int threshold = 0;

	//�ҵ���ֵ����
	threshold = (-ImageArray[(0 + *StartY) * CameraWidth + 0 + *StartX] -
		ImageArray[(0 + *StartY) * CameraWidth + 1 + *StartX] -
		ImageArray[(0 + *StartY) * CameraWidth + 2 + *StartX] -
		ImageArray[(0 + *StartY) * CameraWidth + 3 + *StartX]) / 4 + 15;

	//��ͼ���뵽array[size^2]��,���ƾ��ο��ʱ��ı���startx��ֵ��Ϊ��ʱ���ο�����Ͻ�
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
			if (Array[i*Size + j] < min_value && Array[i*Size + j] != 0)	//���ܰ�0�����Сֵ
				min_value = Array[i*Size + j];
			OriginRoi[i*Size + j] = ImageArray[(i + *StartY) * CameraWidth + j + *StartX];
		}
		UArrayRow[i] = 0;
		UArrayCol[i] = 0;
	}

	//��˹ƽ��
	//	GaussianFilter(Array,Gauss,Size,Size);
	//	memcpy ( Array, Gauss, Size*Size*sizeof(unsigned char) );  //��������  
	GaussianFilter(OriginRoi, Gauss, Size, Size);
	memcpy(OriginRoi, Gauss, Size*Size * sizeof(unsigned char));

	//��һ��У��
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

	//�ֱ�ÿһ�У�ÿһ�еĻҶ�ֵ��ӣ�Ȼ����ƽ��
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
	////	}while((fabs(OffsetY)>0.5)||(fabs(OffsetX)>0.5));//�����ĵ�ԭ��ľ���С��1ʱ���������Ϊ�ҵ����Ľ���
	*PosiX = TempPosX + SIZE / 2;
	*PosiY = TempPosY + SIZE / 2;
	//*PosiX = TempPosX;					//��ʱ��΢������λ��	//���������ֵ
	//*PosiY = TempPosY;

	double Xpos = OffsetX + SIZE / 2;					//��ʱ��΢������λ��,��ʱ��Ϊ���λ��
	double Ypos = OffsetY + SIZE / 2;

	double	linesum[SIZE / 2 + 1] = { 0.0 };	//126���ֱ�Ϊ126��ͬ��Բ���ĻҶ���ֵ�Լ������ص����
	double	linenum[SIZE / 2 + 1] = { 0.0 };
	double	d;					//distance
	int		ds;
	double	coef, ccoef;
	double sum = 0;

	//ת��Ϊ����ʸ��
	for (int i = 0; i < SIZE; i++)		//250
	{
		for (int j = 0; j < SIZE; j++)
		{
			//dΪ��ǰ��������ĵ�Xpos,Ypos�ľ��룬ds����ȡ����
			d = sqrt((double(i) - Xpos)*(double(i) - Xpos) + (double(j) - Ypos)*(double(j) - Ypos));
			ds = (int)floor(d);
			if (d < SIZE / 2)			//û�г���ͼ��Χ
			{
				ccoef = d - ds;			//�ֱ�Ϊ�ڻ��⻷��ռ����,���Ĭ��Ϊ1������
				coef = 1 - ccoef;
				linesum[ds] = linesum[ds] + OriginRoi[j*SIZE + i] * coef;		//image�Ѿ�ת��Ϊһά���顾250*250��
				linesum[ds + 1] = linesum[ds + 1] + OriginRoi[j*SIZE + i] * ccoef;
				//linesum[ds]=linesum[ds]+Array[jp*SIZE+ip]*coef;		//image�Ѿ�ת��Ϊһά���顾250*250��
				//linesum[ds+1]=linesum[ds+1]+Array[jp*SIZE+ip]*ccoef;
				linenum[ds] = linenum[ds] + coef;
				linenum[ds + 1] = linenum[ds + 1] + ccoef;
			}
		}
	}
	for (int i = 0; i < SIZE / 2 + 1; i++)
	{
		line[i] = linesum[i] / linenum[i];			//�����ÿ��Բ����ƽ���Ҷ�ֵ��
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

//����Zλ��
void MeasureBead::MeasureZ(double *ModelMatrix, double *line, double StartPosition, double EndPosition, double StepSize, double *PosiZ)
{
	int			kmin;
	long double Score[MaxModelSize] = { 0 };			//901��ͼƬ���
	long double temp;
	long double Tempmin;
	double 		peakZ;
	int			ModelNum = int((EndPosition - StartPosition) / StepSize + 1);

	//���ܽ�У��ģ���������е�ģ����line�Ƚϣ�score[iz],��¼��У��ģ��ÿһ��ͼ����ѡ���һ��������Ĳ�,�ҵ���Сֵ��Ȼ�������Tempmin��kmin
	for (int iz = 0; iz<ModelNum; iz++)			//level =1Ŀǰ��У��ģ���ܹ���modelnum��,���Զ���level
	{
		for (int jz = 0; jz<MODELSIZE; jz++)				//126,
		{
			temp = line[jz] - ModelMatrix[iz*MODELSIZE + jz];	// ModelMatrix�Ĵ�СΪ901*126 
			Score[iz] += (temp*temp);
		}
		//���tempmin
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
				kmin = iz;			//��ʾ�ڼ���ͼƬ��У��ģ��
			}
		}
	}

	//Ȼ����Ҫ�ҵ�m-1,m+1�ŵ����ݣ�Ȼ�󴫹�ȥ�Ƚ�
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
		//˵��ͼ����0-1֮�䣬�������Ĳ�ֵ����	
		peakZ = 0;
	}
	else
	{
		peakZ = ModelNum;
	}
	*PosiZ = (StartPosition + peakZ*StepSize); //Position Z  	
}

//��ͨ��˹�˲�
void MeasureBead::GaussianFilter(uint8_t* Src, uint8_t* Dest, int width, int height)
{
	int templates[9] = { 1, 1, 1 ,
		1, 1, 1,
		1, 1, 1, };        //�˲���ģ��  
	memcpy(Dest, Src, width*height * sizeof(uint8_t));  //��������  
	for (int j = 2; j<height - 2; j++)  //��Ե������  
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
//�˲���������  
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

//IIR�͸�˹�˲�    
void MeasureBead::FastGaussianFilter(unsigned char* Src, unsigned char* Dest, float B, float *b)
{
	float *Buff, *BPointer, *w1, *w2;

	Buff = (float*)malloc(sizeof(float)*SIZE*SIZE);//���仺��ռ�  
	if (Buff == NULL)
	{
		AfxMessageBox(_T("����ʧ�ܣ��궿�ӣ�"));
		return;
	}
	//����ԭͼ������Buff  
	memcpy(Buff, Src, SIZE*SIZE * sizeof(unsigned char));  //��������  

														   //�����˲�  
	BPointer = Buff;  //ָ��ָ��buff��λ��
					  //����
	w1 = (float*)malloc(sizeof(float)*(SIZE + 4));
	w2 = (float*)malloc(sizeof(float)*(SIZE + 4));
	for (int Y = 0; Y<SIZE; Y++)
	{
		//ǰ���˲�  
		w1[0] = w1[1] = w1[2] = BPointer[0];
		for (int n = 3, i = 0; i<SIZE; n++, i++)
		{
			w1[n] = B*BPointer[i] + (b[1] * w1[n - 1] + b[2] * w1[n - 2] + b[3] * w1[n - 3]);
		}
		//�����˲�  
		w2[SIZE] = w2[SIZE + 1] = w2[SIZE + 2] = w1[SIZE + 2];
		for (int n = SIZE - 1; n >= 0; n--)
		{
			BPointer[n] = w2[n] = B*w1[n + 3] + (b[1] * w2[n + 1] + b[2] * w2[n + 2] + b[3] * w2[n + 3]);
		}
		BPointer += SIZE;
	}

	//�����˲�  
	BPointer = Buff;
	w1 = (float*)realloc(w1, sizeof(float)*(SIZE + 3));
	w2 = (float*)realloc(w2, sizeof(float)*(SIZE + 3));
	for (int X = 0; X<SIZE; X++)
	{
		//ǰ���˲�  
		w1[0] = w1[1] = w1[2] = BPointer[0];
		for (int n = 3, i = 0; i<SIZE; n++, i++)
		{
			w1[n] = B*BPointer[i*SIZE] + (b[1] * w1[n - 1] + b[2] * w1[n - 2] + b[3] * w1[n - 3]);
		}
		//�����˲�  
		w2[SIZE] = w2[SIZE + 1] = w2[SIZE + 2] = w1[SIZE + 2];
		for (int n = SIZE - 1; n >= 0; n--)
		{
			BPointer[n*SIZE] = w2[n] = B*w1[n + 3] + (b[1] * w2[n + 1] + b[2] * w2[n + 2] + b[3] * w2[n + 3]);
		}
		BPointer++;
	}

	//��������Buff����� 
	memcpy(Dest, Buff, SIZE*SIZE * sizeof(unsigned char));  //��������  
	free(w1);
	free(w2);
}

double MeasureBead::KalmanFilter(double ResrcData, double ControlValue, double ProcessNiose_Q, double MeasureNoise_R, double InitialPrediction, long FrequencyDisp, int Channel)
{
	//ȷ����һ�εĿ�����,�Ͳ���ֵ��ͬ�����ٵ�������
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
		//��һ��ϵͳ���Ż�ֵ����һ���м����
		X_mid = X_last + XControlAdd;		//x_last=x(k-1|k-1),x_mid=x(k|k-1)
		Xp_mid = Xp_last + ProcessNiose_Q;	//p_mid=p(k|k-1),p_last=p(k-1|k-1),Q=����

		Xkg = Xp_mid / (Xp_mid + MeasureNoise_R);	//kgΪkalman filter��RΪ����

		X_now = X_mid + Xkg*(ResrcData - X_mid);	//���Ƴ�������ֵ 
		Xp_now = (1 - Xkg)*Xp_mid;//����ֵ��Ӧ��covariance  

								  //����ϵͳ���Ż�ֵ��Э����
		Xp_last = Xp_now; //����covarianceֵ
		X_last = X_now; //����ϵͳ״ֵ̬
		XControlValue_last = ControlValue; //���¿���ֵ
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
		//��һ��ϵͳ���Ż�ֵ����һ���м����
		Y_mid = Y_last + YControlAdd;		//Y_last=Y(k-1|k-1),Y_mid=Y(k|k-1)
		Yp_mid = Yp_last + ProcessNiose_Q;	//p_mid=p(k|k-1),p_last=p(k-1|k-1),Q=����

		Ykg = Yp_mid / (Yp_mid + MeasureNoise_R);	//kgΪkalman filter��RΪ����

		Y_now = Y_mid + Ykg*(ResrcData - Y_mid);	//���Ƴ�������ֵ 
		Yp_now = (1 - Ykg)*Yp_mid;//����ֵ��Ӧ��covariance  

								  //����ϵͳ���Ż�ֵ��Э����
		Yp_last = Yp_now; //����covarianceֵ
		Y_last = Y_now; //����ϵͳ״ֵ̬
		YControlValue_last = ControlValue; //���¿���ֵ
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
		//��һ��ϵͳ���Ż�ֵ����һ���м����
		Z_mid = Z_last + ZControlAdd;		//Z_last=Z(k-1|k-1),Z_mid=Z(k|k-1)
		Zp_mid = Zp_last + ProcessNiose_Q;	//p_mid=p(k|k-1),p_last=p(k-1|k-1),Q=����

		Zkg = Zp_mid / (Zp_mid + MeasureNoise_R);	//kgΪkalman filter��RΪ����

		Z_now = Z_mid + Zkg*(ResrcData - Z_mid);	//���Ƴ�������ֵ 
		Zp_now = (1 - Zkg)*Zp_mid;//����ֵ��Ӧ��covariance  

								  //����ϵͳ���Ż�ֵ��Э����
		Zp_last = Zp_now; //����covarianceֵ
		Z_last = Z_now; //����ϵͳ״ֵ̬
		ZControlValue_last = ControlValue; //���¿���ֵ
		return Z_now;
	}
	}

}
