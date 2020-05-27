
// TilePathFinder.h : ͷ�ļ�
//

#pragma once

#include <stdint.h>
#include <vector>
#include <map>
#include "TilePathFinder.h"
// CTilePathFinderDlg �Ի���
class CTilePathFinderDlg : public CDialogEx {
	// ����
public:
	CTilePathFinderDlg(CWnd* pParent = NULL);	// ��׼���캯��

	// �Ի�������
	enum { IDD = IDD_TILE_FINDPATH_DIALOG };

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
	afx_msg void OnFindPath();
	afx_msg void OnStraightLine();
	afx_msg void OnChangeX();
	afx_msg void OnChangeZ();
	afx_msg void OnChangeEditBrush();

	bool Between(CPoint& pos);
	void UpdateDialog();

	void RayCast(int type);

	void DrawTile(CDC* cdc, int x, int z);

	void DrawBlock(CDC* cdc, CPoint& pos, uint8_t val);

	void DrawBegin();

	void DrawOver();

	static void OnSearchDump(void* userdata, int x, int z);

	static void OnLineDump(void* userdata, int x, int z);
public:

	int m_begin_x;
	int m_begin_z;

	int m_over_x;
	int m_over_z;

	int m_offset_x;
	int m_offset_z;
	int m_scale;
	int m_brush_size;

	bool m_show_path_search;
	bool m_show_line_search;
	bool m_smooth_head;
	bool m_smooth_tail;

	bool m_drag_l;
	bool m_drag_r;
	std::vector<POINT*> m_path;

	std::vector<CDC*> m_cdc;

	bool bNeedPaint;

	static CBrush* pBrushR;
	static CBrush* pBrushG;
	static CBrush* pBrushB;
	static CBrush* pBrushGray;
	static CBrush* pBrushDump;
	static CBrush* pBrushLine;
	static CBrush* pBrushStop;
	static CPen* pPenLine;
	static CPen* pPenLine1;
	static CPen* pPenLine2;

	float m_cost;

	TilePathFinder* m_tile_finder;

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
	afx_msg void OnMouseMove(UINT nFlags, CPoint point);
	afx_msg void OnLButtonDown(UINT nFlags, CPoint point);
	afx_msg void OnRButtonDown(UINT nFlags, CPoint point);
	afx_msg BOOL OnMouseWheel(UINT nFlags, short zDelta, CPoint pt);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
	afx_msg void OnBnClickedCheck3();
	afx_msg void OnBnClickedCheck4();
};
