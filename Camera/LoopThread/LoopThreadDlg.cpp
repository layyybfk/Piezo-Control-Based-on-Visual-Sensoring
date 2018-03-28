
// LoopThreadDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "LoopThread.h"
#include "LoopThreadDlg.h"
#include "afxdialogex.h"
#include "MeasureBead.h"
#include <stdint.h>
#include <fstream>


#include "conio.h" //����̨��� &LTD

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

//ȫ�ֱ��� &LTD
uint8_t *pImageBuffer;
uint8_t *pImageBuffer_;
int StartX = 100;	//ROI��ʼ��
int StartY = 100;
int StartX_ = 100;	//ROI��ʼ��
int StartY_ = 100;
MeasureBead bead_1;
MeasureBead bead_2;
ofstream position_1;
ofstream position_2;
HANDLE m_MeasureThreadEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// �Ի�������
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

// ʵ��
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


// CLoopThreadDlg �Ի���



CLoopThreadDlg::CLoopThreadDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(IDD_LOOPTHREAD_DIALOG, pParent)
	, set_move(0)
	, set_movestep(0)
	, get_v(0)
	, get_move(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

	//��ͼ����ʾ�й� &LTD
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


// CLoopThreadDlg ��Ϣ�������

BOOL CLoopThreadDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
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

	// ���ô˶Ի����ͼ�ꡣ  ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO: �ڴ���Ӷ���ĳ�ʼ������	
	AllocConsole();//�򿪿���̨ &LTD
	GrabThread = AfxBeginThread(ThreadGrab, this, 0, 0, CREATE_SUSPENDED, NULL);
	MeasureBeadThread = AfxBeginThread(ThreadMeasureBead, this, 0, 0, CREATE_SUSPENDED, NULL);
	PIDispThread = AfxBeginThread(ThreadPIDisp, this, 0, 0, CREATE_SUSPENDED, NULL);//��ʾPI�߳�
	PIMotionControlThread = AfxBeginThread(ThreadPIMotionControl, this, 0, 0, CREATE_SUSPENDED, NULL);//PI�˶������߳�

	//��ͼ����ʾ�й� &LTD
	pStc = (CStatic *)GetDlgItem(IDC_IMAGE);//IDC_VIEWΪPicture�ؼ�ID  
	pStc->GetClientRect(&rect);//��CWind��ͻ���������㴫������  
	pDC = pStc->GetDC(); //�õ�Picture�ؼ��豸������  
	hDC = pDC->GetSafeHdc(); //�õ��ؼ��豸�����ĵľ�� 


	GetDlgItem(IDC_PI_STEP_MOVE)->EnableWindow(FALSE);
	GetDlgItem(IDC_PI_MOVE)->EnableWindow(FALSE);
	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
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

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ  ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CLoopThreadDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CLoopThreadDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void CLoopThreadDlg::OnBnClickedInit()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	//AfxMessageBox(_T("��ʼ��"));
	//Pylon::PylonAutoInitTerm autoInitTerm;
	try
	{
		PylonInitialize();//��ʼ���ڳ�ʼ���ĺ���
						  //CInstantCamera camera(CTlFactory::GetInstance().CreateFirstDevice());
		m_camera = new CInstantCamera(CTlFactory::GetInstance().CreateFirstDevice());
		//m_camera->RegisterConfiguration(new CSoftwareTriggerConfiguration, RegistrationMode_ReplaceAll, Cleanup_Delete);
		m_camera->RegisterConfiguration(new CAcquireContinuousConfiguration, RegistrationMode_ReplaceAll, Cleanup_Delete);
		//����AOI��
		m_camera->RegisterConfiguration(new CPixelFormatAndAoiConfiguration, RegistrationMode_Append, Cleanup_Delete);
		// Open the camera device.
		m_camera->Open();
		//if (m_camera->CanWaitForFrameTriggerReady())
		//	m_camera->StartGrabbing(GrabStrategy_OneByOne, GrabLoop_ProvidedByUser);

		_cprintf("Initialization succeed!\n");
	}
	catch (GenICam::GenericException &e)
	{
		// ������.
		AfxMessageBox(_T("��ʼ������"));
		m_camera->Close();
		PylonTerminate();
	}
	//AfxMessageBox(_T("��ʼ��"));
}


void CLoopThreadDlg::OnBnClickedGrab()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	GrabThread->ResumeThread();
}

void CLoopThreadDlg::OnBnClickedMeasure()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	position_1.open("C:/Users/layyybfk/Desktop/data/position_1.dat");
	position_2.open("C:/Users/layyybfk/Desktop/data/position_2.dat");
	MeasureBeadThread->ResumeThread();
}

void CLoopThreadDlg::OnBnClickedExit()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	MeasureBeadThread->SuspendThread();
	AfxMessageBox(_T("�˳�����"));
	m_camera->Close();
	PylonTerminate();
	FreeConsole();
	SendMessage(WM_CLOSE, 0, 0);//�ر�ҳ��
}

void CLoopThreadDlg::OnBnClickedStop()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
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
			// ������.
			AfxMessageBox(_T("�ɼ�����"));
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
		//����ɾ��һ���߳�
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
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	CString r;
	GetDlgItemText(IDC_PI_CONNECT, r);
	if (r == "Connect")
	{
		//���İ���״̬��������λ�ƣ��Լ����ӶϿ�״̬
		GetDlgItem(IDC_PI_STEP_MOVE)->EnableWindow(TRUE);
		GetDlgItem(IDC_PI_MOVE)->EnableWindow(TRUE);
		GetDlgItem(IDC_PI_CONNECT)->SetWindowText(_T("Disconnect"));

		ID = PI_ConnectRS232(4, 115200);

		if (ID<0)
		{
			AfxMessageBox(_T("ID���䲻�ɹ���"));
			PIMotionControlThread->SuspendThread();
			//PIDispThread->SuspendThread();
			PI_CloseConnection(ID);//�رմ���
			GetDlgItem(IDC_PI_STEP_MOVE)->EnableWindow(FALSE);
			GetDlgItem(IDC_PI_MOVE)->EnableWindow(FALSE);
			GetDlgItem(IDC_PI_CONNECT)->SetWindowText(_T("Connect"));
			CButton* pBtn = (CButton*)GetDlgItem(IDC_PI_SERVO_CHECK);	//����ѡ��Ϊѡ��״̬
			pBtn->SetCheck(0);
		}
		if (!PI_qSAI(ID, szAxes, 16))	//Get the identifiers for all configured axes
		{
			AfxMessageBox(_T("�궿�ӣ�"));
			PIMotionControlThread->SuspendThread();
			//PIDispThread->SuspendThread();
			PI_CloseConnection(ID);//�رմ���
			GetDlgItem(IDC_PI_STEP_MOVE)->EnableWindow(FALSE);
			GetDlgItem(IDC_PI_MOVE)->EnableWindow(FALSE);
			GetDlgItem(IDC_PI_CONNECT)->SetWindowText(_T("Connect"));
			CButton* pBtn = (CButton*)GetDlgItem(IDC_PI_SERVO_CHECK);	//����ѡ��Ϊѡ��״̬
			pBtn->SetCheck(0);
		}

		//�����ŷ�ϵͳ��״̬==========================
		BOOL bFlags[1];
		BOOL get_PI_SVO = PI_qSVO(ID, szAxes, bFlags);
		if (bFlags[0] == TRUE)
		{
			CButton* pBtn = (CButton*)GetDlgItem(IDC_PI_SERVO_CHECK);	//����ѡ��Ϊѡ��״̬
			pBtn->SetCheck(1);
			get_SVO = true;
		}
		else//���û�����ջ���������
		{
			OnBnClickedPiServoCheck();
		}

		//���������Ļ���������ʾ�̲߳����¶Ի������
		PIMotionControlThread->ResumeThread();
		PIDispThread->ResumeThread();
	}
	else
	{
		PIMotionControlThread->SuspendThread();
		PIDispThread->SuspendThread();
		PI_CloseConnection(ID);//�رմ���
		GetDlgItem(IDC_PI_STEP_MOVE)->EnableWindow(FALSE);
		GetDlgItem(IDC_PI_MOVE)->EnableWindow(FALSE);
		GetDlgItem(IDC_PI_CONNECT)->SetWindowText(_T("Connect"));
		CButton* pBtn = (CButton*)GetDlgItem(IDC_PI_SERVO_CHECK);	//����ѡ��Ϊѡ��״̬
		pBtn->SetCheck(0);
	}
}


void CLoopThreadDlg::OnBnClickedPiServoCheck()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	BOOL bFlags[1];
	bFlags[0] = TRUE;
	BOOL get_PI_SVO;
	if (BST_CHECKED == IsDlgButtonChecked(IDC_PI_SERVO_CHECK))	//�����������
	{
		get_PI_SVO = PI_SVO(ID, szAxes, bFlags);	//����
		if (!get_PI_SVO)	//����������ɹ�
		{
			AfxMessageBox(_T("�����ջ�ʧ��"));
		}
		else
		{
			get_SVO = true;
		}
	}
	else	//���û�а���
	{
		bFlags[0] = false;
		get_PI_SVO = PI_SVO(ID, szAxes, bFlags);
		get_SVO = false;
	}
}


void CLoopThreadDlg::OnBnClickedPiStepMove()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	PICurrentPos = set_move;
	UpdateData(true);		//��ȡ�Ի���
	PIStepMoveFlag = TRUE;
}


void CLoopThreadDlg::OnBnClickedPiMove()
{
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	UpdateData(true);		//��ȡ�Ի���
	PIOneMoveFlag = TRUE;
}

/*Set Move : ���λ�ƣ�ʹ�þ�ͷ�ƶ�  *//*Get Move : �õ�״̬����ѹ  */
void CLoopThreadDlg::SetMove(double setmove)
{
	double dPos[1];
	double get_po[1];
	get_po[0] = 0;

	dPos[0] = setmove / 1000;		//position 
									//����λ�Ʒ�ʽ`````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````````
	if (get_SVO)		//����Ǳջ�ģʽ
	{
		if (!PI_MOV(ID, szAxes, dPos))		//������ִ�У�PI_MOVΪ�ƶ���ָ��λ��dDos
		{
			AfxMessageBox(_T("�ƶ�ʧ��"));
		}
	}
	else			//����ǿ���
	{
		if (!PI_SVA(ID, szAxes, dPos))	//Set absolute open-loop control value to move the axis
		{
			AfxMessageBox(_T("�ƶ�ʧ��"));
		}
	}
}

void CLoopThreadDlg::GetMove(void)
{
	//�õ���ѹ
	double get_Vol[3];
	double get_po[1];
	get_po[0] = 0;
	int PI_piezo[1];
	PI_piezo[0] = NULL;

	BOOL get_Vo = PI_qVOL(ID, PI_piezo, get_Vol, 0);	//Get current piezo voltages for piPiezoChannelsArray.
	get_v = get_Vol[0];						//�Ի��򣬽�����ʾ��ѹ

											//�õ�״̬
	BOOL bIsMoving[1];
	bIsMoving[0] = TRUE;
	while (bIsMoving[0] == TRUE)
	{
		if (!PI_qPOS(ID, szAxes, get_po))		//Get Real Position
		{
			AfxMessageBox(_T("��ȡλ��ʧ��"));
		}
		if (!PI_IsMoving(ID, NULL, bIsMoving))		//�Ƿ��ƶ��ˣ�
		{
			AfxMessageBox(_T("��ȡ״̬ʧ��"));
		}
	}
	get_move = 1000 * get_po[0];	//��������ʾ��ǰ��λ��
	CString PosPI = _T(""); PosPI.Format(_T("%.3f"), get_move);
	SetDlgItemText(IDC_GET_MOVE, PosPI);
	CString VolPI = _T(""); VolPI.Format(_T("%.3f"), get_v);
	SetDlgItemText(IDC_GET_V, VolPI);
}

void CLoopThreadDlg::OnDeltaposSpinPi(NMHDR *pNMHDR, LRESULT *pResult)
{
	LPNMUPDOWN pNMUpDown = reinterpret_cast<LPNMUPDOWN>(pNMHDR);
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	UpdateData(true);
	if (pNMUpDown->iDelta == 1) //�����ֵΪ1 , ˵�������Spin�����¼�ͷ
	{
		set_move -= set_movestep;
		if (set_move<0)
		{
			set_move = 0;
			AfxMessageBox(_T("Exceeded the lower limit"));
			CString SetPos = _T(""); SetPos.Format(_T("%d"), set_move);//&LTD�޸Ĺ�
			SetDlgItemText(IDC_SET_MOVE, SetPos);
		}
		else
		{
			CString SetPos = _T(""); SetPos.Format(_T("%d"), set_move);
			SetDlgItemText(IDC_SET_MOVE, SetPos);
		}
	}
	else if (pNMUpDown->iDelta == -1) // �����ֵΪ-1 , ˵�������Spin�����ϼ�ͷ
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

//==PI��ͷ������λ�ÿ����߳�
UINT ThreadPIMotionControl(LPVOID lpParam)
{
	CLoopThreadDlg* p = (CLoopThreadDlg*)lpParam;
	int PIMotion = 0;
	int PIStepSize = 0;
	while (true) //һֱѭ��
	{
		if (p->get_SVO)	//ֻ���ڿ����ջ�����ʱ��ſ���
		{
			if (p->PIOneMoveFlag)
			{
				p->SetMove(p->set_move);
				p->PIOneMoveFlag = FALSE;
			}
			if (p->PIStepMoveFlag)
			{
				//��ȡ��ǰλ��
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
				//��ʼ�ƶ�
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
	// TODO: �ڴ���ӿؼ�֪ͨ����������
	//OnBnClickedFreeze();//��ֹͣ�ɼ�
	//UpdateData(true);

	////����ǵ�һ��΢���ģ�ͽ���
	//if (m_judge_bead == 1)
	//{
	//	int CurrentPosition = 0;
	//	if (set_move % set_movestep != 0)
	//	{
	//		AfxMessageBox(_T("PI-Z����λ�Ʋ�����������������������"));
	//		return;
	//	}
	//	int StepNumber = set_move / set_movestep;		//PI�Ĳ���

	//													//ѭ��������ѭ��
	//	for (int step = 0; step <= StepNumber; step++)
	//	{
	//		/*�˶���ĳһ��λ�� */
	//		CurrentPosition = set_movestep*step;
	//		SetMove((double)CurrentPosition);
	//		Sleep(500);//�˶���ֹͣ500ms������

	//				   /*������λ�õ�ģ��*/
	//		for (int m = 0; m < ImageNumber; m++)
	//		{
	//			m_Xfer->Snap();
	//			BOOL result = m_Buffers->GetAddress((void**)&Buf);	//�õ���ַ

	//																//��ͼ��ת��Ϊ����ʸ��line
	//			Meas.CalMod(Buf, &StartX, &StartY, line, &PosiX, &PosiY);//�Ե���ͼƬ���д���ת��Ϊ����ʸ��line

	//			CWnd *pWin = GetDlgItem(IDC_VIEW_WND);
	//			CDC *pDc = pWin->GetDC();

	//			//����ѡ���
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

	//			//����m_imagenum��ͼƬ�ľ���ʸ��line֮��
	//			for (int i = 0; i < MatrixCol; i++)
	//				Sumline[i] += line[i] / ImageNumber;
	//		}//ѭ��image-num�κ�������ɼ���ͼ��

	//		 /*��У��ģ�ʹ����ļ��У���������е�һ��*/
	//		ofstream CalliModel;
	//		CalliModel.open("C:/Users/dell/Desktop/exp/MatrixModelBelow.dat", ios::app);//ios::app���û���ļ������ɿ��ļ���������ļ������ļ�β׷��
	//		for (int i = 0; i < MatrixCol; i++)
	//		{
	//			CalliModel << Sumline[i] << "\t";
	//		}
	//		CalliModel << endl;
	//		CalliModel.close();

	//		//�����浥λ��0,
	//		memset(Sumline, 0, sizeof(Sumline));
	//	}	//step����

	//		//��󽫽����õ�ģ�ʹ��뵽�������Թ�Ӧ��
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
	//		fclose(finMode);						//�رմ򿪵��ļ�
	//	}
	//}//if   bead==1����


	//if (m_judge_bead == 2)//�ж�˫���������
	//{
	//	//��ǰλ�þ������õ�λ�ã��ڴ˻������½���ģ��
	//	int StepNumber = 50;
	//	int CurrentPosition = set_move - 20000;
	//	SetMove((double)CurrentPosition);
	//	Sleep(1000);

	//	if (set_move % set_movestep != 0)
	//	{
	//		AfxMessageBox(_T("PI-Z����λ�Ʋ�����������������������"));
	//		return;
	//	}

	//	//ѭ��������ѭ��
	//	for (int step = 0; step <= StepNumber; step++)
	//	{
	//		/*�˶���ĳһ��λ�� */
	//		CurrentPosition = CurrentPosition + set_movestep;
	//		SetMove((double)CurrentPosition);
	//		Sleep(500);//�˶���ֹͣ500ms������

	//				   /*������λ�õ�ģ��*/
	//		for (int m = 0; m <ImageNumber; m++)
	//		{
	//			m_Xfer->Snap();
	//			BOOL result = m_Buffers->GetAddress((void**)&Buf);	//�õ���ַ

	//																//��ͼ��ת��Ϊ����ʸ��line
	//																//Meas.CalMod(Buf,&StartX,&StartY,line,&PosiX,&PosiY);//�Ե���ͼƬ���д���ת��Ϊ����ʸ��line

	//			Meashalf.HalfbeadCalMod(Buf, &StartX, &StartY, &PosiX, &PosiY, line);

	//			CWnd *pWin = GetDlgItem(IDC_VIEW_WND);
	//			CDC *pDc = pWin->GetDC();

	//			//����ѡ���
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

	//			//����m_imagenum��ͼƬ�ľ���ʸ��line֮��
	//			for (int i = 0; i < MatrixCol; i++)
	//				Sumline[i] = Sumline[i] + line[i] / ImageNumber;
	//		}//ѭ��image-num�κ�������ɼ���ͼ��

	//		 /*��У��ģ�ʹ����ļ��У���������е�һ��*/
	//		ofstream CalliModel;
	//		CalliModel.open("C:/Users/dell/Desktop/exp/MatrixModelTop.dat", ios::app);//ios::app���û���ļ������ɿ��ļ���������ļ������ļ�β׷��
	//		for (int i = 0; i < MatrixCol; i++)
	//		{
	//			CalliModel << Sumline[i] << "\t";
	//		}
	//		CalliModel << endl;
	//		CalliModel.close();

	//		//�����浥λ��0,
	//		memset(Sumline, 0, sizeof(Sumline));
	//	}	//step����

	//		//��󽫽����õ�ģ�ʹ��뵽�������Թ�Ӧ��
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
	//		fclose(finMode);						//�رմ򿪵��ļ�
	//	}
	//}//m_judge_bead==2 ΢��ģ�ͽ������

	//m_Xfer->Snap();
	//AfxMessageBox(_T("ģ�ͽ������"));
}
