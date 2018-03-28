
// LoopThreadDlg.h : 头文件
//
#pragma once

//1.包含头文件并定义图像处理函数 &LTD
#include <pylon/PylonIncludes.h>
#include <pylon/InstantCamera.h>

// Include files used by samples. &LTD
#include "../include/ConfigurationEventPrinter.h"
#include "../include/ImageEventPrinter.h"
#include "../include/PixelFormatAndAoiConfiguration.h" //用于配置采集AOI和输出格式等
#include "afxwin.h"

#include "PI_GCS2_DLL.h"//PI驱动 &LTD
//和Opencv相关的头文件 &LTD
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

// CLoopThreadDlg 对话框
class CLoopThreadDlg : public CDialogEx
{
// 构造
public:
	CLoopThreadDlg(CWnd* pParent = NULL);	// 标准构造函数


// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_LOOPTHREAD_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


// 实现
protected:
	HICON m_hIcon;

	public :
	Pylon::CInstantCamera * m_camera = nullptr;

protected:
	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	//与显示相关 &LTD
	CRect rect;
	CStatic* pStc; //标识图像显示的Picture控件  
	CDC* pDC; //视频显示控件设备上下文  
	HDC hDC; //视频显示控件设备句柄      
	CvCapture* capture; //视频获取结构  
	CBitmap cbitmap;
	cv::Mat img_mat;

	CStatic m_image;
	CWinThread* GrabThread;
	CWinThread* MeasureBeadThread;

	int ID;	//设备ID
	char szAxes[17];
	bool get_SVO;
	CWinThread* PIDispThread;
	CWinThread* PIMotionControlThread;

	int set_movestep;//界面的movestep PI的步长
	int set_move;
	double get_move;
	double get_v;//对话框的output voltage 
	BOOL PIStepMoveFlag;
	BOOL PIOneMoveFlag;
	int PICurrentPos;			//当前PI的位置
	

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

//创建线程
UINT ThreadGrab(LPVOID lpParam);
UINT ThreadMeasureBead(LPVOID lpParam);

UINT ThreadPIDisp(LPVOID lpParam);
UINT ThreadPIMotionControl(LPVOID lpParam);