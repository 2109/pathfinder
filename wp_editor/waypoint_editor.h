
// waypoint_editor.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CWpEditorApp: 
// �йش����ʵ�֣������ waypoint_editor.cpp
//

class CWpEditorApp : public CWinApp
{
public:
	CWpEditorApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CWpEditorApp theApp;