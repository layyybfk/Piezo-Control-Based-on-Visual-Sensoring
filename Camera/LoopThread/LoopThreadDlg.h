
// LoopThreadDlg.h : ͷ�ļ�
//
#pragma once

//1.����ͷ�ļ�������ͼ������ &LTD
#include <pylon/PylonIncludes.h>
#include <pylon/InstantCamera.h>

// Include files used by samples. &LTD
#include "../include/ConfigurationEventPrinter.h"
#include "../include/ImageEventPrinter.h"
#include "../include/PixelFormatAndAoiConfiguration.h" //�������òɼ�AOI�������ʽ��
#include "afxwin.h"

#include "PI_GCS2_DLL.h"//PI���� &LTD
//��Opencv��ص�ͷ�ļ� &LTD
#include "opencv/cv.h"
#include "opencv/highgui.h"
#include "opencv2/opencv.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/core/core.hpp"
//#include <cxcore.h>
#include <cv.h>
#include <highgui.h>
#include "CvvImage.h"

// CLoopThreadDlg �Ի���
class CLoopThreadDlg : public CDialogEx
{
// ����
public:
	CLoopThreadDlg(CWnd* pParent = NULL);	// ��׼���캯��


// �Ի�������
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_LOOPTHREAD_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��


// ʵ��
protected:
	HICON m_hIcon;

	public :
	Pylon::CInstantCamera * m_camera = nullptr;

protected:
	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	//����ʾ��� &LTD
	CRect rect;
	CStatic* pStc; //��ʶͼ����ʾ��Picture�ؼ�  
	CDC* pDC; //��Ƶ��ʾ�ؼ��豸������  
	HDC hDC; //��Ƶ��ʾ�ؼ��豸���      
	CvCapture* capture; //��Ƶ��ȡ�ṹ  
	CBitmap cbitmap;
	cv::Mat img_mat;

	CStatic m_image;
	CWinThread* GrabThread;
	CWinThread* MeasureBeadThread;

	int ID;	//�豸ID
	char szAxes[17];
	bool get_SVO;
	CWinThread* PIDispThread;
	CWinThread* PIMotionControlThread;

	int set_movestep;//�����movestep PI�Ĳ���
	int set_move;
	double get_move;
	double get_v;//�Ի����output voltage 
	BOOL PIStepMoveFlag;
	BOOL PIOneMoveFlag;
	int PICurrentPos;			//��ǰPI��λ��
	

	void GetMove(void);
	void SetMove(double setmove);

	afx_msg void OnBnClickedInit();
	afx_msg void OnBnClickedGrab();
	afx_msg void OnBnClickedExit();
	afx_msg void OnBnClickedStop();
	afx_msg void OnBnClickedMeasure();

	afx_msg void OnBnClickedPiConnect();
	afx_msg void OnBnClickedPiServoCheck();
	afx_msg void OnBnClickedPiStepMove();
	afx_msg void OnBnClickedPiMove();
	afx_msg void OnDeltaposSpinPi(NMHDR *pNMHDR, LRESULT *pResult);
	afx_msg void OnBnClickedButtonCalibration();
};

//�����߳�
UINT ThreadGrab(LPVOID lpParam);
UINT ThreadMeasureBead(LPVOID lpParam);

UINT ThreadPIDisp(LPVOID lpParam);
UINT ThreadPIMotionControl(LPVOID lpParam);