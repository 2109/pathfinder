
// waypoint_editor.h : PROJECT_NAME Ӧ�ó������ͷ�ļ�
//

#pragma once

#ifndef __AFXWIN_H__
	#error "�ڰ������ļ�֮ǰ������stdafx.h�������� PCH �ļ�"
#endif

#include "resource.h"		// ������


// CWaypointEditorApp: 
// �йش����ʵ�֣������ waypoint_editor.cpp
//

class CWaypointEditorApp : public CWinApp
{
public:
	CWaypointEditorApp();

// ��д
public:
	virtual BOOL InitInstance();

// ʵ��

	DECLARE_MESSAGE_MAP()
};

extern CWaypointEditorApp theApp;