
// wp_finder.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// Cwp_finderApp: 
// �йش����ʵ�֣������ wp_finder.cpp
//

class Cwp_finderApp : public CWinApp
{
public:
	Cwp_finderApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern Cwp_finderApp theApp;