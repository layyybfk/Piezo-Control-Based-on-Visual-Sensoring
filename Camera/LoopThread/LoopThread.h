
// LoopThread.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CLoopThreadApp: 
// �йش����ʵ�֣������ LoopThread.cpp
//

class CLoopThreadApp : public CWinApp
{
public:
	CLoopThreadApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CLoopThreadApp theApp;