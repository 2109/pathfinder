
// wp_finder.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CWpFinderApp: 
// �йش����ʵ�֣������ wp_finder.cpp
//

class CWpFinderApp : public CWinApp
{
public:
	CWpFinderApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CWpFinderApp theApp;