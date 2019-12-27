
// TilePathFinder.h : 头文件
//

#pragma once

#include <stdint.h>
#include <vector>
extern "C"{
#include "pathfinder.h"
}

#include "TilePathFinderEx.h"
// CTilePathFinderDlg 对话框
class CTilePathFinderDlg : public CDialogEx
{
// 构造
public:
	CTilePathFinderDlg(CWnd* pParent = NULL);	// 标准构造函数

// 对话框数据
	enum { IDD = IDD_TILE_FINDPATH_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持


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
	afx_msg void OnFindPath();
	afx_msg void OnStraightLine();
	afx_msg void OnChangeX();
	afx_msg void OnChangeZ();
	afx_msg void OnChangeScale();

	bool Between(CPoint& pos);
	void UpdateDialog();

	void RayCast(int type);

	void DrawBegin();

	void DrawOver();
public:
	int m_width;
	int m_heigh;
	uint8_t* m_data;
	int m_size;

	int m_begin_x;
	int m_begin_z;

	int m_over_x;
	int m_over_z;

	int m_offset_x;
	int m_offset_z;
	int m_scale;

	bool m_show_path_search;
	bool m_show_line_search;
	bool m_edit;

	std::vector<POINT*> m_path;

	float m_cost;

	TilePathFinder* m_tile_finder;
	//struct pathfinder* m_finder;
	CStatic* m_time_cost;
	CStatic* m_pos_start;
	CStatic* m_pos_over;
public:
//	afx_msg void OnLButtonDblClk(UINT nFlags, CPoint point);
	afx_msg void OnLButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnUpdateIddTileFindpathDialog(CCmdUI *pCmdUI);
	afx_msg void OnEnChangeEdit4();
	afx_msg void OnClose();
	afx_msg void OnBnClickedPathCheck();
	afx_msg void OnBnClickedLineCheck();
	afx_msg void OnBnClickedEditCheck();
//	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnStraightLineEx();
	afx_msg void OnRandomPos();
};
