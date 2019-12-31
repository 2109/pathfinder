
// PathFinderTestDlg.h : ͷ�ļ�
//

#pragma once



#include "NavMeshFinder.h"
#include <vector>
// CNavDlg �Ի���
class CNavDlg : public CDialogEx
{
// ����
public:
	CNavDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_PATHFINDERTEST_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV ֧��

// ʵ��
protected:
	HICON m_hIcon;

	// ���ɵ���Ϣӳ�亯��
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnPath();
	afx_msg int OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message);

	void DrawMap();
	void DrawBegin(CPoint& pos);
	void DrawOver(CPoint& pos);

	void DrawPath(std::vector<const Math::Vector3*>& path);

public:
	NavPathFinder* m_finder;
	int m_offset_x;
	int m_offset_z;
	double m_scale;
	int m_scale_base;
	bool m_mouse_state;
	CPoint* m_mouse_point;

	int m_poly_begin;
	Math::Vector3* m_pt_begin;
	int m_poly_over;
	Math::Vector3* m_pt_over;

	std::vector<CDC*> m_cdc;
public:
	afx_msg void OnUpdateIddPathfindertestDialog(CCmdUI *pCmdUI);
	afx_msg void Straightline();
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	
	afx_msg void OnIgnoreLine();
	afx_msg void OnEnChangeEdit2();
	afx_msg void OnEnChangeEdit3();
	afx_msg void OnEnChangeEdit4();
	afx_msg void OnIgnorePath();
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnClose();
	afx_msg void OnCheck();
	afx_msg void OnEnChangeEdit5();
	afx_msg void OnEnChangeEdit6();
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
};
