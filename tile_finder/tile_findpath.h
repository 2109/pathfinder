
// tile_findpath.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CTilePathFinderApp:
// �йش����ʵ�֣������ tile_findpath.cpp
//

class CTilePathFinderApp : public CWinApp
{
public:
	CTilePathFinderApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CTilePathFinderApp theApp;