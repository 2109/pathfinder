
// wp_finderDlg.h : 头文件
//

#pragma once

#include <vector>
#include <set>
#include <map>
#include "navmesh_finder.h"
#include "wp_path_finder.h"

struct WayPoint {
	std::set<int> link;
	CPoint pt;
	int id;
	int r;
	bool check;

	bool Inside(CPoint& point) {
		int dist_x = point.x - pt.x;
		int dist_y = point.y - pt.y;
		if (sqrt(dist_x*dist_x + dist_y * dist_y) <= r) {
			return true;
		}
		return false;
	}
};

// CWpFinderDlg 对话框
class CWpFinderDlg : public CDialogEx {
	// 构造
public:
	CWpFinderDlg(CWnd* pParent = NULL);	// 标准构造函数

	// 对话框数据
	enum { IDD = IDD_WP_FINDER_DIALOG };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

public:
	void UpdateMesh();
	void editor2mesh(CPoint* from, CPoint* to);
	void mesh2editor(CPoint* from, CPoint* to);

public:
	NavPathFinder* m_mesh_finder;
	int m_offset_x;
	int m_offset_z;
	double m_scale;
	bool m_mouse_state;
	CPoint* m_mouse_point;

	std::map<int, WayPoint*> m_wp_list;

	WpPathFinder* m_finder;
	CPoint* m_start;
	CPoint* m_over;
	std::vector<CPoint> m_path;
	std::vector<CPoint> m_search;

	// 实现
protected:
	HICON m_hIcon;

	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg void OnMButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnMButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnBnClickedButton1();
};
