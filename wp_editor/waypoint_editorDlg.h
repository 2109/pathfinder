
// waypoint_editorDlg.h : ͷ�ļ�
//

#pragma once
#include <vector>
#include <set>
#include <map>
#include "NavMeshFinder.h"

struct WayPoint {
	std::set<int> link;
	CPoint pt;
	int id;
	int r;
	bool check;

	bool Inside(CPoint& point) {
		int dist_x = point.x - pt.x;
		int dist_y = point.y - pt.y;
		if ( sqrt(dist_x*dist_x + dist_y * dist_y) <= r ) {
			return true;
		}
		return false;
	}
};

// CWpEditorDlg �Ի���
class CWpEditorDlg : public CDialogEx
{
// ����
public:
	CWpEditorDlg(CWnd* pParent = NULL);	// ��׼���캯��

// �Ի�������
	enum { IDD = IDD_WAYPOINT_EDITOR_DIALOG };

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

	void UpdateMesh();
	void editor2mesh(CPoint& from, CPoint& to);
	void mesh2editor(CPoint& from, CPoint& to);
private:
	NavPathFinder* m_finder;
	int m_offset_x;
	int m_offset_z;
	double m_scale;
	bool m_mouse_state;
	CPoint* m_mouse_point;

	
	int m_last_wp;
	int m_id_countor;
	std::map<int,WayPoint*> m_wp_list;
public:
	afx_msg void OnUpdateIddWaypointEditorDialog(CCmdUI *pCmdUI);
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
//	afx_msg void OnMouseHWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
//	afx_msg void OnBnClickedCheck1();
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnBnClickedButton1();
	afx_msg void OnMouseHWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnBnClickedButton2();
};
