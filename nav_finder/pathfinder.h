
// PathFinderTest.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// pathfinderApp: 
// �йش����ʵ�֣������ PathFinderTest.cpp
//

class pathfinderApp : public CWinApp
{
public:
	pathfinderApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern pathfinderApp theApp;