
// TilePathFinder.cpp : 实现文件
//

#include "stdafx.h"
#include "tile_findpath.h"
#include "TilePathFinderDlg.h"
#include "afxdialogex.h"

#include <io.h>    
#include <fcntl.h>  

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx {
public:
	CAboutDlg();

	// 对话框数据
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

	// 实现
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD) {
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX) {
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CTileCNavDlg 对话框

CBrush* CTileCNavDlg::pBrushR = new CBrush(RGB(255, 0, 0));
CBrush* CTileCNavDlg::pBrushG = new CBrush(RGB(0, 255, 0));
CBrush* CTileCNavDlg::pBrushB = new CBrush(RGB(0, 0, 255));

CTileCNavDlg::CTileCNavDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CTileCNavDlg::IDD, pParent) {
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CTileCNavDlg::DoDataExchange(CDataExchange* pDX) {
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CTileCNavDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CTileCNavDlg::OnFindPath)
	ON_BN_CLICKED(IDC_BUTTON2, &CTileCNavDlg::OnStraightLine)
	ON_EN_CHANGE(IDC_EDIT1, &CTileCNavDlg::OnChangeX)
	ON_EN_CHANGE(IDC_EDIT2, &CTileCNavDlg::OnChangeZ)
	ON_EN_CHANGE(IDC_EDIT3, &CTileCNavDlg::OnChangeScale)
	//	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONUP()
	ON_UPDATE_COMMAND_UI(IDD_TILE_FINDPATH_DIALOG, &CTileCNavDlg::OnUpdateIddTileFindpathDialog)
	ON_EN_CHANGE(IDC_EDIT4, &CTileCNavDlg::OnEnChangeEdit4)
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_CHECK1, &CTileCNavDlg::OnBnClickedPathCheck)
	ON_BN_CLICKED(IDC_CHECK2, &CTileCNavDlg::OnBnClickedLineCheck)
	ON_BN_CLICKED(IDC_CHECK3, &CTileCNavDlg::OnBnClickedEditCheck)
	//ON_WM_LBUTTONDOWN()
	ON_BN_CLICKED(IDC_BUTTON3, &CTileCNavDlg::OnStraightLineEx)
	ON_BN_CLICKED(IDC_BUTTON4, &CTileCNavDlg::OnRandomPos)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
END_MESSAGE_MAP()


// CTileCNavDlg 消息处理程序

BOOL CTileCNavDlg::OnInitDialog() {
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL) {
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty()) {
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码

	m_offset_x = 200;
	m_offset_z = 10;
	m_scale = 5;
	m_cost = 50;
	CString str;
	str.Format(_T("%d"), m_offset_x);
	((CEdit*)GetDlgItem(IDC_EDIT1))->SetWindowTextW(str);
	str.Format(_T("%d"), m_offset_z);
	((CEdit*)GetDlgItem(IDC_EDIT2))->SetWindowTextW(str);
	str.Format(_T("%d"), m_scale);
	((CEdit*)GetDlgItem(IDC_EDIT3))->SetWindowTextW(str);
	str.Format(_T("%f"), m_cost);
	((CEdit*)GetDlgItem(IDC_EDIT4))->SetWindowTextW(str);

	m_begin_x = m_begin_z = -1;
	m_over_x = m_over_z = -1;

	CString tile_file;
	tile_file.Format(_T("./tile/%s"), AfxGetApp()->m_lpCmdLine);

	m_show_path_search = false;
	m_show_line_search = false;
	m_edit = false;

	((CButton*)GetDlgItem(IDC_CHECK1))->SetCheck(m_show_path_search);
	((CButton*)GetDlgItem(IDC_CHECK1))->SetCheck(m_show_line_search);
	((CButton*)GetDlgItem(IDC_CHECK3))->SetCheck(m_edit);

	USES_CONVERSION;
	char * pFileName = T2A(tile_file);
	m_tile_finder = TilePathFinder::LoadFromFile(pFileName);

	m_time_cost = new CStatic();
	m_time_cost->Create(_T(""), WS_CHILD | WS_VISIBLE | SS_CENTER, CRect(10, 20, 150, 100), this);

	m_pos_start = new CStatic();
	m_pos_start->Create(_T(""), WS_CHILD | WS_VISIBLE | SS_CENTER, CRect(10, 50, 150, 100), this);

	m_pos_over = new CStatic();
	m_pos_over->Create(_T(""), WS_CHILD | WS_VISIBLE | SS_CENTER, CRect(10, 80, 150, 100), this);

	AllocConsole();
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	int hCrt = _open_osfhandle((long)handle, _O_TEXT);
	FILE * hf = _fdopen(hCrt, "w");
	*stdout = *hf;


	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CTileCNavDlg::OnSysCommand(UINT nID, LPARAM lParam) {
	if ((nID & 0xFFF0) == IDM_ABOUTBOX) {
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	} else {
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CTileCNavDlg::OnPaint() {
	if (IsIconic()) {
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	} else {
		CDialogEx::OnPaint();
	}

	UpdateDialog();
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CTileCNavDlg::OnQueryDragIcon() {
	return static_cast<HCURSOR>(m_hIcon);
}

struct DumpArgs {
	CTileCNavDlg* self;
	CClientDC* cdc;
};

void OnSearchDump(void* ud, int x, int z) {
	DumpArgs* args = (DumpArgs*)ud;
	CTileCNavDlg* self = args->self;
	CClientDC* cdc = args->cdc;

	CBrush brush_begin(RGB(0, 0, 0));
	CBrush* obrush = cdc->SelectObject(&brush_begin);

	int index = x * self->m_tile_finder->GetHeight() + z;

	CPoint pt[4];
	pt[0].x = x * self->m_scale + self->m_offset_x;
	pt[0].y = z * self->m_scale + self->m_offset_z;
	pt[1].x = (x + 1) * self->m_scale + self->m_offset_x;
	pt[1].y = z * self->m_scale + self->m_offset_z;
	pt[2].x = (x + 1) * self->m_scale + self->m_offset_x;
	pt[2].y = (z + 1) * self->m_scale + self->m_offset_z;
	pt[3].x = x * self->m_scale + self->m_offset_x;
	pt[3].y = (z + 1) * self->m_scale + self->m_offset_z;

	cdc->Polygon(pt, 4);

	cdc->SelectObject(obrush);

	Sleep(1);
}

void OnPathDump(void* ud, int x, int z) {
	DumpArgs* args = (DumpArgs*)ud;
	CTileCNavDlg* self = args->self;
	CClientDC* cdc = args->cdc;

	POINT* pt = new POINT();
	pt->x = x;
	pt->y = z;
	self->m_path.push_back(pt);
}

void OnPathDummy(void* ud, int x, int z) {
}

void LineDump(void* ud, int x, int z) {
	DumpArgs* args = (DumpArgs*)ud;
	CTileCNavDlg* self = args->self;
	CClientDC* cdc = args->cdc;

	CBrush brush(RGB(0, 255, 0));
	POINT from;
	from.x = x * self->m_scale + self->m_offset_x;
	from.y = z * self->m_scale + self->m_offset_z;
	POINT to;
	to.x = from.x + self->m_scale;
	to.y = from.y + self->m_scale;

	cdc->FillRect(CRect(from, to), &brush);
	Sleep(10);
}

void CTileCNavDlg::OnFindPath() {
	// TODO: 在此添加控件通知处理程序代码
	m_path.clear();

	CClientDC dc(this);
	DumpArgs args;
	args.self = this;
	args.cdc = &dc;

	LARGE_INTEGER freq;
	LARGE_INTEGER start, over;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&start);

	m_tile_finder->SetDebugCallback(m_show_path_search ? OnSearchDump : NULL, &args);

	Math::Vector2 from(m_begin_x, m_begin_z);
	Math::Vector2 to(m_over_x, m_over_z);

	std::vector<const Math::Vector2*> list;
	m_tile_finder->Find(from, to, true, list, m_cost);
	for (int i = 0; i < list.size(); i++) {
		POINT* pt = new POINT();
		pt->x = list[i]->x;
		pt->y = list[i]->y;
		m_path.push_back(pt);
	}

	QueryPerformanceCounter(&over);

	CPen pen(PS_SOLID, 1, RGB(100, 100, 255));
	CPen *open = dc.SelectObject(&pen);

	if (m_path.size() != 0) {
		std::vector<POINT*>::iterator iter = m_path.begin();
		POINT pt;
		pt.x = ((*iter)->x + 0.5) * m_scale + m_offset_x;
		pt.y = ((*iter)->y + 0.5) * m_scale + m_offset_z;
		dc.MoveTo(pt);

		for (iter++; iter != m_path.end(); iter++) {
			pt.x = ((*iter)->x + 0.5) * m_scale + m_offset_x;
			pt.y = ((*iter)->y + 0.5) * m_scale + m_offset_z;

			dc.LineTo(pt);
			dc.MoveTo(pt);
		}
	}

	dc.SelectObject(open);


	double pathCost = (double)((over.QuadPart - start.QuadPart) * 1000) / (double)freq.QuadPart;
	CString str;
	str.Format(_T("耗时:%fms"), pathCost);
	m_time_cost->SetWindowText(str);
}

void CTileCNavDlg::OnChangeX() {
	CString str;
	((CEdit*)GetDlgItem(IDC_EDIT1))->GetWindowTextW(str);
	m_offset_x = _ttoi(str);
	Invalidate();
}

void CTileCNavDlg::OnChangeZ() {
	CString str;
	((CEdit*)GetDlgItem(IDC_EDIT2))->GetWindowTextW(str);
	m_offset_z = _ttoi(str);
	Invalidate();
}


void CTileCNavDlg::OnChangeScale() {
	CString str;
	((CEdit*)GetDlgItem(IDC_EDIT3))->GetWindowTextW(str);
	m_scale = _ttoi(str);
	Invalidate();
}

bool CTileCNavDlg::Between(CPoint& pos) {
	if (pos.x >= m_offset_x && pos.y >= m_offset_z) {
		if (pos.x <= (m_tile_finder->GetWidth() - 1) * m_scale + m_offset_x && pos.y <= (m_tile_finder->GetHeight() - 1) * m_scale + m_offset_z) {
			return true;
		}
	}
	return false;
}

void CTileCNavDlg::UpdateDialog() {
	CClientDC dc(this);
	CPen pen(PS_SOLID, 1, RGB(0, 0, 0));
	CPen *oopen = dc.SelectObject(&pen);
	CBrush brush0(RGB(255, 0, 0));
	CBrush brush1(RGB(0, 255, 0));
	CBrush brush2(RGB(0, 0, 255));

	for (int x = 0; x < m_tile_finder->GetWidth(); x++) {
		for (int z = 0; z < m_tile_finder->GetHeight(); z++) {

			CBrush* obrush;
			if (m_tile_finder->GetBlock(x, z) == 1)
				obrush = dc.SelectObject(pBrushG);
			else if (m_tile_finder->GetBlock(x, z) == 0)
				obrush = dc.SelectObject(pBrushR);
			else
				obrush = dc.SelectObject(pBrushB);

			DrawTile(dc, x, z);

			dc.SelectObject(obrush);
		}
	}
	dc.SelectObject(oopen);

	DrawBegin();
	DrawOver();
}


void CTileCNavDlg::OnLButtonUp(UINT nFlags, CPoint point) {
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	m_drag_l = false;
	if (Between(point)) {
		if (m_edit) {
			int x = (point.x - m_offset_x) / m_scale;
			int z = (point.y - m_offset_z) / m_scale;
			m_tile_finder->SetBlock(x, z, 1);
			CClientDC dc(this);
			CBrush brush0(RGB(255, 0, 0));
			CBrush* ob = dc.SelectObject(&brush0);
			DrawTile(dc, x, z);
			dc.SelectObject(ob);
		} else {
			m_begin_x = (point.x - m_offset_x) / m_scale;
			m_begin_z = (point.y - m_offset_z) / m_scale;

			DrawBegin();

			CClientDC dc(this);
			DumpArgs args;
			args.self = this;
			args.cdc = &dc;

			//m_tile_finder->SetDebugCallback(OnSearchDump, &args);

			//for (int i = 0; i < 1000; i++) {
			//	Math::Vector2 result;
			//	m_tile_finder->RandomInCircle(m_begin_x, m_begin_z, 16, result);
			//	dc.SetPixel((result.x + (Math::Rand((float)0, (float)1))) * (float)m_scale + m_offset_x, (result.y + (Math::Rand((float)0, (float)1))) * (float)m_scale + m_offset_z, RGB(255, 255, 250));
			//}
		}
	}

	CDialogEx::OnLButtonUp(nFlags, point);
}


void CTileCNavDlg::OnRButtonUp(UINT nFlags, CPoint point) {
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	m_drag_r = false;
	if (Between(point)) {
		if (m_edit) {
			int x = (point.x - m_offset_x) / m_scale;
			int z = (point.y - m_offset_z) / m_scale;
			m_tile_finder->SetBlock(x, z, 2);
			CClientDC dc(this);
			CBrush brush2(RGB(0, 0, 255));
			CBrush* ob = dc.SelectObject(&brush2);
			DrawTile(dc, x, z);
			dc.SelectObject(ob);
		} else {
			m_over_x = (point.x - m_offset_x) / m_scale;
			m_over_z = (point.y - m_offset_z) / m_scale;

			if (m_tile_finder->IsBlock(m_over_x, m_over_z)) {
				CClientDC dc(this);
				DumpArgs args;
				args.self = this;
				args.cdc = &dc;
				m_tile_finder->SetDebugCallback(OnSearchDump, &args);
				TilePathFinder::PathNode* node = m_tile_finder->SearchInCircle(m_over_x, m_over_z, TilePathFinder::kSearchDepth);
				if (!node) {
					node = m_tile_finder->SearchInReactangle(m_over_x, m_over_z, 16);
				}
				if (node) {
					m_over_x = node->pos_.x;
					m_over_z = node->pos_.y;
					DrawOver();
				}
			} else {
				DrawOver();
			}
		}

		//Invalidate();
	}

	CDialogEx::OnRButtonUp(nFlags, point);
}


void CTileCNavDlg::OnUpdateIddTileFindpathDialog(CCmdUI *pCmdUI) {
	// TODO: 在此添加命令更新用户界面处理程序代码
	UpdateDialog();
}


void CTileCNavDlg::OnEnChangeEdit4() {
	CString str;
	((CEdit*)GetDlgItem(IDC_EDIT4))->GetWindowTextW(str);
	m_cost = _ttoi(str);
}


void CTileCNavDlg::OnClose() {
	//finder_release(m_finder);

	//FILE* fd = fopen("nav.tile", "wb+");
	//fwrite(&m_width, 1, sizeof(int), fd);
	//fwrite(&m_heigh, 1, sizeof(int), fd);
	//fwrite(m_data, 1, m_size, fd);
	//fclose(fd);

	CDialogEx::OnClose();
}


void CTileCNavDlg::OnBnClickedPathCheck() {
	m_show_path_search = ((CButton*)GetDlgItem(IDC_CHECK1))->GetCheck();
}


void CTileCNavDlg::OnBnClickedLineCheck() {
	// TODO:  在此添加控件通知处理程序代码
	m_show_line_search = ((CButton*)GetDlgItem(IDC_CHECK2))->GetCheck();
}


void CTileCNavDlg::OnBnClickedEditCheck() {
	// TODO:  在此添加控件通知处理程序代码
	m_edit = ((CButton*)GetDlgItem(IDC_CHECK3))->GetCheck();
}

void CTileCNavDlg::OnStraightLine() {
	// TODO: 在此添加控件通知处理程序代码
	RayCast(0);
}


void CTileCNavDlg::OnStraightLineEx() {
	// TODO:  在此添加控件通知处理程序代码
	RayCast(1);
}

void CTileCNavDlg::RayCast(int type) {
	CClientDC dc(this);
	DumpArgs args;
	args.self = this;
	args.cdc = &dc;

	int rx = 0;
	int ry = 0;

	LARGE_INTEGER freq;
	LARGE_INTEGER start, over;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&start);

	m_tile_finder->SetDebugCallback(m_show_line_search ? LineDump : NULL, &args);

	const Math::Vector2 fromPos(m_begin_x, m_begin_z);
	const Math::Vector2 toPos(m_over_x, m_over_z);
	Math::Vector2 result;
	Math::Vector2 stop;

	if (type == 0) {
		m_tile_finder->Raycast(fromPos, toPos, false, &result, &stop);
	} else {
		m_tile_finder->Raycast(fromPos, toPos, false, &result, &stop);
	}


	QueryPerformanceCounter(&over);

	double pathCost = (double)((over.QuadPart - start.QuadPart) * 1000) / (double)freq.QuadPart;
	CString str;
	str.Format(_T("耗时:%fms"), pathCost);
	m_time_cost->SetWindowText(str);

	POINT from;
	from.x = (m_begin_x + 0.5) * m_scale + m_offset_x;
	from.y = (m_begin_z + 0.5) * m_scale + m_offset_z;
	POINT to;
	to.x = (result.x + 0.5) * m_scale + m_offset_x;
	to.y = (result.y + 0.5) * m_scale + m_offset_z;
	dc.MoveTo(from);
	dc.LineTo(to);

	CBrush brush_begin(RGB(123, 255, 87));
	CBrush* obrush = dc.SelectObject(&brush_begin);
	DrawTile(dc, stop.x, stop.y);
	dc.SelectObject(obrush);
}

extern "C" void finder_random(struct pathfinder* finder, int* x, int* z);

void CTileCNavDlg::OnRandomPos() {
	// TODO:  在此添加控件通知处理程序代码
	CClientDC cdc(this);

	for (int i = 0; i < 1000; i++) {
		Math::Vector2 result;
		m_tile_finder->Random(result);
		POINT pt;
		pt.x = result.x * m_scale + m_offset_x;
		pt.y = result.y * m_scale + m_offset_z;

		cdc.Ellipse(pt.x - 1, pt.y - 1, pt.x + 1, pt.y + 1);
	}
}

void CTileCNavDlg::DrawTile(CClientDC& cdc, int x, int z) {
	CPoint pt[4];
	pt[0].x = x * m_scale + m_offset_x;
	pt[0].y = z * m_scale + m_offset_z;
	pt[1].x = (x + 1) * m_scale + m_offset_x;
	pt[1].y = z * m_scale + m_offset_z;
	pt[2].x = (x + 1) * m_scale + m_offset_x;
	pt[2].y = (z + 1) * m_scale + m_offset_z;
	pt[3].x = x * m_scale + m_offset_x;
	pt[3].y = (z + 1) * m_scale + m_offset_z;

	cdc.Polygon(pt, 4);
}

void CTileCNavDlg::DrawBegin() {
	CClientDC dc(this);

	CString str;
	str.Format(_T("起点:%d,%d"), -1, -1);
	m_pos_start->SetWindowText(str);

	CBrush brush_begin(RGB(0, 255, 0));
	CBrush* obrush = dc.SelectObject(&brush_begin);
	if (m_begin_x != -1 && m_begin_z != -1) {
		DrawTile(dc, m_begin_x, m_begin_z);
		str.Format(_T("起点:%d,%d"), m_begin_x, m_begin_z);
		m_pos_start->SetWindowText(str);
	}
	dc.SelectObject(obrush);
}

void CTileCNavDlg::DrawOver() {
	CClientDC dc(this);

	CString str;
	str.Format(_T("终点:%d,%d"), -1, -1);
	m_pos_over->SetWindowText(str);

	CBrush brush_over(RGB(0, 0, 0));
	CBrush* obrush = dc.SelectObject(&brush_over);
	if (m_over_x != -1 && m_over_z != -1) {
		DrawTile(dc, m_over_x, m_over_z);
		str.Format(_T("终点:%d,%d"), m_over_x, m_over_z);
		m_pos_over->SetWindowText(str);
	}
	dc.SelectObject(obrush);
}

void CTileCNavDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnMouseMove(nFlags, point);
	if (m_edit) {
		if (m_drag_l) {
			int x = (point.x - m_offset_x) / m_scale;
			int z = (point.y - m_offset_z) / m_scale;
			m_tile_finder->SetBlock(x, z, 2);
			CClientDC dc(this);
			CBrush* ob = dc.SelectObject(pBrushG);
			DrawTile(dc, x, z);
			dc.SelectObject(ob);
		}
		if (m_drag_r) {
			int x = (point.x - m_offset_x) / m_scale;
			int z = (point.y - m_offset_z) / m_scale;
			m_tile_finder->SetBlock(x, z, 1);
			CClientDC dc(this);
			CBrush brush2(RGB(0, 0, 255));
			CBrush* ob = dc.SelectObject(pBrushR);
			DrawTile(dc, x, z);
			dc.SelectObject(ob);
		}
	}

}


void CTileCNavDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnLButtonDown(nFlags, point);
	m_drag_l = true;
}


void CTileCNavDlg::OnRButtonDown(UINT nFlags, CPoint point)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnRButtonDown(nFlags, point);
	m_drag_r = true;
}
