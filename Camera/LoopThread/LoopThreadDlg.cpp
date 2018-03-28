
// LoopThreadDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "LoopThread.h"
#include "LoopThreadDlg.h"
#include "afxdialogex.h"
#include "MeasureBead.h"
#include <stdint.h>
#include <fstream>


#include "conio.h" //控制台输出 &LTD

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

#ifdef PYLON_WIN_BUILD
#    include <pylon/PylonGUI.h>
#endif



// Namespace for using pylon objects.
using namespace Pylon;

// Namespace for using cout.
using namespace std;

//全局变量 &LTD
uint8_t *pImageBuffer;
uint8_t *pImageBuffer_;
int StartX = 100;	//ROI开始点
int StartY = 100;
int StartX_ = 100;	//ROI开始点
int StartY_ = 100;
MeasureBead bead_1;
MeasureBead bead_2;
ofstream position_1;
ofstream position_2;
HANDLE m_MeasureThreadEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CLoopThreadDlg 对话框



CLoopThreadDlg::CLoopThreadDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_LOOPTHREAD_DIALOG, pParent)
	, set_move(0)
	, set_movestep(0)
	, get_v(0)
	, get_move(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	//与图像显示有关 &LTD
	pStc = NULL;
	pDC = NULL;
	capture = NULL;
}

void CLoopThreadDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_IMAGE, m_image);
	//  DDX_Text(pDX, IDC_SET_MOVE, set_move);
	DDX_Text(pDX, IDC_SET_MOVE, set_move);
	DDX_Text(pDX, IDC_SET_MOVE_STEP, set_movestep);
	DDX_Text(pDX, IDC_GET_V, get_v);
	DDX_Text(pDX, IDC_GET_MOVE, get_move);
}

BEGIN_MESSAGE_MAP(CLoopThreadDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_INIT, &CLoopThreadDlg::OnBnClickedInit)
	ON_BN_CLICKED(IDC_GRAB, &CLoopThreadDlg::OnBnClickedGrab)
	ON_BN_CLICKED(IDC_EXIT, &CLoopThreadDlg::OnBnClickedExit)
	ON_BN_CLICKED(IDC_STOP, &CLoopThreadDlg::OnBnClickedStop)
	ON_BN_CLICKED(IDC_MEASURE, &CLoopThreadDlg::OnBnClickedMeasure)
	ON_BN_CLICKED(IDC_PI_CONNECT, &CLoopThreadDlg::OnBnClickedPiConnect)
	ON_BN_CLICKED(IDC_PI_SERVO_CHECK, &CLoopThreadDlg::OnBnClickedPiServoCheck)
	ON_BN_CLICKED(IDC_PI_STEP_MOVE, &CLoopThreadDlg::OnBnClickedPiStepMove)
	ON_BN_CLICKED(IDC_PI_MOVE, &CLoopThreadDlg::OnBnClickedPiMove)
	ON_NOTIFY(UDN_DELTAPOS, IDC_SPIN_PI, &CLoopThreadDlg::OnDeltaposSpinPi)
	ON_BN_CLICKED(IDC_BUTTON_CALIBRATION, &CLoopThreadDlg::OnBnClickedButtonCalibration)
END_MESSAGE_MAP()


// CLoopThreadDlg 消息处理程序

BOOL CLoopThreadDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码	
	AllocConsole();//打开控制台 &LTD
	GrabThread = AfxBeginThread(ThreadGrab, this, 0, 0, CREATE_SUSPENDED, NULL);
	MeasureBeadThread = AfxBeginThread(ThreadMeasureBead, this, 0, 0, CREATE_SUSPENDED, NULL);
	PIDispThread = AfxBeginThread(ThreadPIDisp, this, 0, 0, CREATE_SUSPENDED, NULL);//显示PI线程
	PIMotionControlThread = AfxBeginThread(ThreadPIMotionControl, this, 0, 0, CREATE_SUSPENDED, NULL);//PI运动控制线程

	//与图像显示有关 &LTD
	pStc = (CStatic *)GetDlgItem(IDC_IMAGE);//IDC_VIEW为Picture控件ID  
	pStc->GetClientRect(&rect);//将CWind类客户区的坐标点传给矩形  
	pDC = pStc->GetDC(); //得到Picture控件设备上下文  
	hDC = pDC->GetSafeHdc(); //得到控件设备上下文的句柄 


	GetDlgItem(IDC_PI_STEP_MOVE)->EnableWindow(FALSE);
	GetDlgItem(IDC_PI_MOVE)->EnableWindow(FALSE);
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CLoopThreadDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CLoopThreadDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CLoopThreadDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CLoopThreadDlg::OnBnClickedInit()
{
	// TODO: 在此添加控件通知处理程序代码
	//AfxMessageBox(_T("初始化"));
	//Pylon::PylonAutoInitTerm autoInitTerm;
	try
	{
		PylonInitialize();//起始用于初始化的函数
						  //CInstantCamera camera(CTlFactory::GetInstance().CreateFirstDevice());
		m_camera = new CInstantCamera(CTlFactory::GetInstance().CreateFirstDevice());
		//m_camera->RegisterConfiguration(new CSoftwareTriggerConfiguration, RegistrationMode_ReplaceAll, Cleanup_Delete);
		m_camera->RegisterConfiguration(new CAcquireContinuousConfiguration, RegistrationMode_ReplaceAll, Cleanup_Delete);
		//配置AOI等
		m_camera->RegisterConfiguration(new CPixelFormatAndAoiConfiguration, RegistrationMode_Append, Cleanup_Delete);
		// Open the camera device.
		m_camera->Open();
		//if (m_camera->CanWaitForFrameTriggerReady())
		//	m_camera->StartGrabbing(GrabStrategy_OneByOne, GrabLoop_ProvidedByUser);

		_cprintf("Initialization succeed!\n");
	}
	catch (GenICam::GenericException &e)
	{
		// 错误处理.
		AfxMessageBox(_T("初始化出错"));
		m_camera->Close();
		PylonTerminate();
	}
	//AfxMessageBox(_T("初始化"));
}


void CLoopThreadDlg::OnBnClickedGrab()
{
	// TODO: 在此添加控件通知处理程序代码
	GrabThread->ResumeThread();
}

void CLoopThreadDlg::OnBnClickedMeasure()
{
	// TODO: 在此添加控件通知处理程序代码
	position_1.open("C:/Users/layyybfk/Desktop/data/position_1.dat");
	position_2.open("C:/Users/layyybfk/Desktop/data/position_2.dat");
	MeasureBeadThread->ResumeThread();
}

void CLoopThreadDlg::OnBnClickedExit()
{
	// TODO: 在此添加控件通知处理程序代码
	MeasureBeadThread->SuspendThread();
	AfxMessageBox(_T("退出程序"));
	m_camera->Close();
	PylonTerminate();
	FreeConsole();
	SendMessage(WM_CLOSE, 0, 0);//关闭页面
}

void CLoopThreadDlg::OnBnClickedStop()
{
	// TODO: 在此添加控件通知处理程序代码
	//SetEvent(m_MeasureThreadEvent);
	position_1.close();
	position_2.close();
	MeasureBeadThread->SuspendThread();
	GrabThread->SuspendThread();
}
UINT  ThreadGrab(LPVOID lpParam)
{
	CLoopThreadDlg* p = (CLoopThreadDlg*)lpParam;
	p->m_camera->StartGrabbing();
	while (true)
	{
		CGrabResultPtr ptrGrabResult;
		try
		{
			if (p->m_camera->IsGrabbing())
				p->m_camera->RetrieveResult(50, ptrGrabResult, TimeoutHandling_ThrowException);
		}
		catch (GenICam::GenericException &e)
		{
			// 错误处理.
			AfxMessageBox(_T("采集出错"));
			p->m_camera->Close();
			PylonTerminate();
		}
		if (ptrGrabResult->GrabSucceeded())
		{
			pImageBuffer = (uint8_t *)ptrGrabResult->GetBuffer();
			pImageBuffer_ = (uint8_t *)ptrGrabResult->GetBuffer();
			uint32_t bmWidth = ptrGrabResult->GetWidth();
			uint32_t bmHeight = ptrGrabResult->GetHeight();
			
			p->img_mat = cv::Mat(bmHeight, bmWidth, CV_8UC1, (unsigned char*)pImageBuffer);
			IplImage im = IplImage(p->img_mat);
			CvvImage show;
			show.CopyOf(&im);
			show.DrawToHDC(p->hDC, &p->rect);
		}
	}
	return 0;
}

UINT  ThreadMeasureBead(LPVOID lpParam)
{
	CLoopThreadDlg* p = (CLoopThreadDlg*)lpParam;
	
	while (true)
	{
		//用于删除一个线程
		//DWORD flagMeasureThread;
		//flagMeasureThread = WaitForSingleObject(m_MeasureThreadEvent, 50);
		//if (flagMeasureThread == WAIT_TIMEOUT)
		//{
		//	bead_1.MeasureXY(pImageBuffer, &StartX, &StartY, &PosiX, &PosiY);
		//	_cprintf("Position of the bead is: %f", PosiX);
		//}
		//else
		//{
		//	DWORD dwExitCode;
		//	GetExitCodeThread(ThreadMeasureBead, &dwExitCode);
		//	AfxEndThread(dwExitCode, TRUE);
		//}
		//
		bead_1.MeasureXY(pImageBuffer, &StartX, &StartY, &PosiX, &PosiY);
		_cprintf("Position of the bead_1 is: %f\n", PosiX);
		_cprintf("Position of the bead_1 is: %f\n\n", PosiY);
		//position_1 << PosiX << " " << PosiY << endl;
		bead_2.MeasureXY_(pImageBuffer_, &StartX_, &StartY_, &PosiX2, &PosiY2);
		_cprintf("Position of the bead_2 is: %f\n", PosiX2);
		_cprintf("Position of the bead_2 is: %f\n\n", PosiY2);
		//position_2 << PosiX2 << " " << PosiY2 << endl;
	}
	return 0;
}

void CLoopThreadDlg::OnBnClickedPiConnect()
{
	// TODO: 在此添加控件通知处理程序代码
	CString r;
	GetDlgItemText(IDC_PI_CONNECT, r);
	if (r == "Connect")
	{
		//更改按键状态，步进，位移，以及连接断开状态
		GetDlgItem(IDC_PI_STEP_MOVE)->EnableWindow(TRUE);
		GetDlgItem(IDC_PI_MOVE)->EnableWindow(TRUE);
		GetDlgItem(IDC_PI_CONNECT)->SetWindowText(_T("Disconnect"));

		ID = PI_ConnectRS232(4, 115200);

		if (ID<0)
		{
			AfxMessageBox(_T("ID分配不成功！"));
			PIMotionControlThread->SuspendThread();
			//PIDispThread->SuspendThread();
			PI_CloseConnection(ID);//关闭串口
			GetDlgItem(IDC_PI_STEP_MOVE)->EnableWindow(FALSE);
			GetDlgItem(IDC_PI_MOVE)->EnableWindow(FALSE);
			GetDlgItem(IDC_PI_CONNECT)->SetWindowText(_T("Connect"));
			CButton* pBtn = (CButton*)GetDlgItem(IDC_PI_SERVO_CHECK);	//更新选框为选中状态
			pBtn->SetCheck(0);
		}
		if (!PI_qSAI(ID, szAxes, 16))	//Get the identifiers for all configured axes
		{
			AfxMessageBox(_T("完犊子！"));
			PIMotionControlThread->SuspendThread();
			//PIDispThread->SuspendThread();
			PI_CloseConnection(ID);//关闭串口
			GetDlgItem(IDC_PI_STEP_MOVE)->EnableWindow(FALSE);
			GetDlgItem(IDC_PI_MOVE)->EnableWindow(FALSE);
			GetDlgItem(IDC_PI_CONNECT)->SetWindowText(_T("Connect"));
			CButton* pBtn = (CButton*)GetDlgItem(IDC_PI_SERVO_CHECK);	//更新选框为选中状态
			pBtn->SetCheck(0);
		}

		//检验伺服系统的状态==========================
		BOOL bFlags[1];
		BOOL get_PI_SVO = PI_qSVO(ID, szAxes, bFlags);
		if (bFlags[0] == TRUE)
		{
			CButton* pBtn = (CButton*)GetDlgItem(IDC_PI_SERVO_CHECK);	//更新选框为选中状态
			pBtn->SetCheck(1);
			get_SVO = true;
		}
		else//如果没开启闭环则开启即可
		{
			OnBnClickedPiServoCheck();
		}

		//如果都无误的话，开启显示线程并更新对话框控制
		PIMotionControlThread->ResumeThread();
		PIDispThread->ResumeThread();
	}
	else
	{
		PIMotionControlThread->SuspendThread();
		PIDispThread->SuspendThread();
		PI_CloseConnection(ID);//关闭串口
		GetDlgItem(IDC_PI_STEP_MOVE)->EnableWindow(FALSE);
		GetDlgItem(IDC_PI_MOVE)->EnableWindow(FALSE);
		GetDlgItem(IDC_PI_CONNECT)->SetWindowText(_T("Connect"));
		CButton* pBtn = (CButton*)GetDlgItem(IDC_PI_SERVO_CHECK);	//更新选框为选中状态
		pBtn->SetCheck(0);
	}
}


void CLoopThreadDlg::OnBnClickedPiServoCheck()
{
	// TODO: 在此添加控件通知处理程序代码
	BOOL bFlags[1];
	bFlags[0] = TRUE;
	BOOL get_PI_SVO;
	if (BST_CHECKED == IsDlgButtonChecked(IDC_PI_SERVO_CHECK))	//如果被按下了
	{
		get_PI_SVO = PI_SVO(ID, szAxes, bFlags);	//连接
		if (!get_PI_SVO)	//如果开启不成功
		{
			AfxMessageBox(_T("开启闭环失败"));
		}
		else
		{
			get_SVO = true;
		}
	}
	else	//如果没有按下
	{
		bFlags[0] = false;
		get_PI_SVO = PI_SVO(ID, szAxes, bFlags);
		get_SVO = false;
	}
}


void CLoopThreadDlg::OnBnClickedPiStepMove()
{
	// TODO: 在此添加控件通知处理程序代码
	PICurrentPos = set_move;
	UpdateData(true);		//读取对话框
	PIStepMoveFlag = TRUE;
}


void CLoopThreadDlg::OnBnClickedPiMove()
{
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(true);		//读取对话框
	PIOneMoveFlag = TRUE;
}

/*Set Move : 输出位移，使得镜头移动  *//*Get Move : 得到状态，电压  */
void CLoopThreadDlg::SetMove(double setmove)
{
	double dPos[1];
	double get_po[1];
	get_po[0] = 0;

	dPos[0] = setmove / 1000;		//position 
									//两种位移方式`````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````
	if (get_SVO)		//如果是闭环模式
	{
		if (!PI_MOV(ID, szAxes, dPos))		//函数先执行，PI_MOV为移动到指定位置dDos
		{
			AfxMessageBox(_T("移动失败"));
		}
	}
	else			//如果是开环
	{
		if (!PI_SVA(ID, szAxes, dPos))	//Set absolute open-loop control value to move the axis
		{
			AfxMessageBox(_T("移动失败"));
		}
	}
}

void CLoopThreadDlg::GetMove(void)
{
	//得到电压
	double get_Vol[3];
	double get_po[1];
	get_po[0] = 0;
	int PI_piezo[1];
	PI_piezo[0] = NULL;

	BOOL get_Vo = PI_qVOL(ID, PI_piezo, get_Vol, 0);	//Get current piezo voltages for piPiezoChannelsArray.
	get_v = get_Vol[0];						//对话框，界面显示电压

											//得到状态
	BOOL bIsMoving[1];
	bIsMoving[0] = TRUE;
	while (bIsMoving[0] == TRUE)
	{
		if (!PI_qPOS(ID, szAxes, get_po))		//Get Real Position
		{
			AfxMessageBox(_T("获取位置失败"));
		}
		if (!PI_IsMoving(ID, NULL, bIsMoving))		//是否移动了？
		{
			AfxMessageBox(_T("获取状态失败"));
		}
	}
	get_move = 1000 * get_po[0];	//界面上显示当前的位置
	CString PosPI = _T(""); PosPI.Format(_T("%.3f"), get_move);
	SetDlgItemText(IDC_GET_MOVE, PosPI);
	CString VolPI = _T(""); VolPI.Format(_T("%.3f"), get_v);
	SetDlgItemText(IDC_GET_V, VolPI);
}

void CLoopThreadDlg::OnDeltaposSpinPi(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	UpdateData(true);
	if (pNMUpDown->iDelta == 1) //如果此值为1 , 说明点击了Spin的往下箭头
	{
		set_move -= set_movestep;
		if (set_move<0)
		{
			set_move = 0;
			AfxMessageBox(_T("Exceeded the lower limit"));
			CString SetPos = _T(""); SetPos.Format(_T("%d"), set_move);//&LTD修改过
			SetDlgItemText(IDC_SET_MOVE, SetPos);
		}
		else
		{
			CString SetPos = _T(""); SetPos.Format(_T("%d"), set_move);
			SetDlgItemText(IDC_SET_MOVE, SetPos);
		}
	}
	else if (pNMUpDown->iDelta == -1) // 如果此值为-1 , 说明点击了Spin的往上箭头
	{
		set_move += set_movestep;
		if (set_move>100000)
		{
			set_move = 0;
			AfxMessageBox(_T("Exceeded the upper limit"));
			CString SetPos = _T(""); SetPos.Format(_T("%d"), set_move);
			SetDlgItemText(IDC_SET_MOVE, SetPos);
		}
		else
		{
			CString SetPos = _T(""); SetPos.Format(_T("%d"), set_move);
			SetDlgItemText(IDC_SET_MOVE, SetPos);
		}
	}
	*pResult = 0;
}

UINT ThreadPIDisp(LPVOID lpParam)
{
	CLoopThreadDlg* p = (CLoopThreadDlg*)lpParam;
	while (true)
	{
		p->GetMove();
		Sleep(100);
	}
}

//==PI镜头驱动的位置控制线程
UINT ThreadPIMotionControl(LPVOID lpParam)
{
	CLoopThreadDlg* p = (CLoopThreadDlg*)lpParam;
	int PIMotion = 0;
	int PIStepSize = 0;
	while (true) //一直循环
	{
		if (p->get_SVO)	//只有在开启闭环控制时候才可以
		{
			if (p->PIOneMoveFlag)
			{
				p->SetMove(p->set_move);
				p->PIOneMoveFlag = FALSE;
			}
			if (p->PIStepMoveFlag)
			{
				//先取当前位置
				int Error = p->set_move - p->PICurrentPos;
				if (Error>0)
				{
					PIStepSize = p->set_movestep;
				}
				else
				{
					PIStepSize = -p->set_movestep;
				}
				int PIStepNumber = abs(Error / PIStepSize);
				//开始移动
				for (int i = 0; i<PIStepNumber; i++)
				{
					PIMotion = p->PICurrentPos + i*PIStepSize;
					p->SetMove(PIMotion);
					Sleep(10);
				}
				p->SetMove(p->set_move);
				p->PIStepMoveFlag = FALSE;

			}
		}
		Sleep(100);
	}
}

void CLoopThreadDlg::OnBnClickedButtonCalibration()
{
	// TODO: 在此添加控件通知处理程序代码
	//OnBnClickedFreeze();//先停止采集
	//UpdateData(true);

	////如果是第一个微球的模型建立
	//if (m_judge_bead == 1)
	//{
	//	int CurrentPosition = 0;
	//	if (set_move % set_movestep != 0)
	//	{
	//		AfxMessageBox(_T("PI-Z轴总位移不能整除步长，请重新设置"));
	//		return;
	//	}
	//	int StepNumber = set_move / set_movestep;		//PI的步数

	//													//循环，步数循环
	//	for (int step = 0; step <= StepNumber; step++)
	//	{
	//		/*运动到某一个位置 */
	//		CurrentPosition = set_movestep*step;
	//		SetMove((double)CurrentPosition);
	//		Sleep(500);//运动完停止500ms减少震荡

	//				   /*建立该位置的模型*/
	//		for (int m = 0; m < ImageNumber; m++)
	//		{
	//			m_Xfer->Snap();
	//			BOOL result = m_Buffers->GetAddress((void**)&Buf);	//得到地址

	//																//将图像转化为径向矢量line
	//			Meas.CalMod(Buf, &StartX, &StartY, line, &PosiX, &PosiY);//对单张图片进行处理，转换为径向矢量line

	//			CWnd *pWin = GetDlgItem(IDC_VIEW_WND);
	//			CDC *pDc = pWin->GetDC();

	//			//绘制选择框
	//			CPen pen(PS_SOLID, 3, RGB(100, 255, 200));
	//			CPen *pOldPen = pDc->SelectObject(&pen);
	//			CBrush *pBrush = CBrush::FromHandle((HBRUSH)GetStockObject(NULL_BRUSH));
	//			CBrush *pOldBrush = pDc->SelectObject(pBrush);
	//			pDc->Ellipse((int)PosiX - 120, (int)PosiY - 120, (int)PosiX + 120, (int)PosiY + 120);
	//			pDc->Ellipse((int)PosiX - 75, (int)PosiY - 75, (int)PosiX + 75, (int)PosiY + 75);
	//			pDc->Ellipse((int)PosiX - 30, (int)PosiY - 30, (int)PosiX + 30, (int)PosiY + 30);
	//			pDc->Ellipse((int)PosiX - 1, (int)PosiY - 1, (int)PosiX + 1, (int)PosiY + 1);
	//			pDc->SelectObject(pOldPen);
	//			pDc->SelectObject(pOldBrush);
	//			ReleaseDC(pDc);

	//			//计算m_imagenum张图片的径向矢量line之和
	//			for (int i = 0; i < MatrixCol; i++)
	//				Sumline[i] += line[i] / ImageNumber;
	//		}//循环image-num次后结束，采集到图像

	//		 /*将校正模型存入文件中，存的是其中的一行*/
	//		ofstream CalliModel;
	//		CalliModel.open("C:/Users/dell/Desktop/exp/MatrixModelBelow.dat", ios::app);//ios::app如果没有文件，生成空文件；如果有文件，在文件尾追加
	//		for (int i = 0; i < MatrixCol; i++)
	//		{
	//			CalliModel << Sumline[i] << "\t";
	//		}
	//		CalliModel << endl;
	//		CalliModel.close();

	//		//将储存单位置0,
	//		memset(Sumline, 0, sizeof(Sumline));
	//	}	//step结束

	//		//最后将建立好的模型存入到数组中以供应用
	//	FILE* finMode;
	//	char  *finModename = "C:/Users/dell/Desktop/exp/MatrixModelBelow.dat";
	//	fopen_s(&finMode, finModename, "rt");
	//	int iCount1 = 0;
	//	if (finMode != NULL)
	//	{
	//		do {
	//			fscanf_s(finMode, "%lf ", &ModelMatrixBelow[iCount1]);
	//			iCount1++;
	//		} while (!feof(finMode));
	//		fclose(finMode);						//关闭打开的文件
	//	}
	//}//if   bead==1结束


	//if (m_judge_bead == 2)//判断双球，上面的球
	//{
	//	//当前位置就是设置的位置，在此基础上下建立模型
	//	int StepNumber = 50;
	//	int CurrentPosition = set_move - 20000;
	//	SetMove((double)CurrentPosition);
	//	Sleep(1000);

	//	if (set_move % set_movestep != 0)
	//	{
	//		AfxMessageBox(_T("PI-Z轴总位移不能整除步长，请重新设置"));
	//		return;
	//	}

	//	//循环，步数循环
	//	for (int step = 0; step <= StepNumber; step++)
	//	{
	//		/*运动到某一个位置 */
	//		CurrentPosition = CurrentPosition + set_movestep;
	//		SetMove((double)CurrentPosition);
	//		Sleep(500);//运动完停止500ms减少震荡

	//				   /*建立该位置的模型*/
	//		for (int m = 0; m <ImageNumber; m++)
	//		{
	//			m_Xfer->Snap();
	//			BOOL result = m_Buffers->GetAddress((void**)&Buf);	//得到地址

	//																//将图像转化为径向矢量line
	//																//Meas.CalMod(Buf,&StartX,&StartY,line,&PosiX,&PosiY);//对单张图片进行处理，转换为径向矢量line

	//			Meashalf.HalfbeadCalMod(Buf, &StartX, &StartY, &PosiX, &PosiY, line);

	//			CWnd *pWin = GetDlgItem(IDC_VIEW_WND);
	//			CDC *pDc = pWin->GetDC();

	//			//绘制选择框
	//			CPen pen(PS_SOLID, 3, RGB(100, 255, 200));
	//			CPen *pOldPen = pDc->SelectObject(&pen);
	//			CBrush *pBrush = CBrush::FromHandle((HBRUSH)GetStockObject(NULL_BRUSH));
	//			CBrush *pOldBrush = pDc->SelectObject(pBrush);
	//			pDc->Ellipse((int)PosiX - 120, (int)PosiY - 120, (int)PosiX + 120, (int)PosiY + 120);
	//			pDc->Ellipse((int)PosiX - 75, (int)PosiY - 75, (int)PosiX + 75, (int)PosiY + 75);
	//			pDc->Ellipse((int)PosiX - 30, (int)PosiY - 30, (int)PosiX + 30, (int)PosiY + 30);
	//			pDc->Ellipse((int)PosiX - 1, (int)PosiY - 1, (int)PosiX + 1, (int)PosiY + 1);
	//			pDc->SelectObject(pOldPen);
	//			pDc->SelectObject(pOldBrush);
	//			ReleaseDC(pDc);

	//			//计算m_imagenum张图片的径向矢量line之和
	//			for (int i = 0; i < MatrixCol; i++)
	//				Sumline[i] = Sumline[i] + line[i] / ImageNumber;
	//		}//循环image-num次后结束，采集到图像

	//		 /*将校正模型存入文件中，存的是其中的一行*/
	//		ofstream CalliModel;
	//		CalliModel.open("C:/Users/dell/Desktop/exp/MatrixModelTop.dat", ios::app);//ios::app如果没有文件，生成空文件；如果有文件，在文件尾追加
	//		for (int i = 0; i < MatrixCol; i++)
	//		{
	//			CalliModel << Sumline[i] << "\t";
	//		}
	//		CalliModel << endl;
	//		CalliModel.close();

	//		//将储存单位置0,
	//		memset(Sumline, 0, sizeof(Sumline));
	//	}	//step结束

	//		//最后将建立好的模型存入到数组中以供应用
	//	FILE* finMode;
	//	char  *finModename = "C:/Users/dell/Desktop/exp/MatrixModelBelow.dat";
	//	fopen_s(&finMode, finModename, "rt");
	//	int iCount1 = 0;
	//	if (finMode != NULL)
	//	{
	//		do {
	//			fscanf_s(finMode, "%lf ", &ModelMatrixTop[iCount1]);
	//			iCount1++;
	//		} while (!feof(finMode));
	//		fclose(finMode);						//关闭打开的文件
	//	}
	//}//m_judge_bead==2 微球模型建立完毕

	//m_Xfer->Snap();
	//AfxMessageBox(_T("模型建立完毕"));
}
