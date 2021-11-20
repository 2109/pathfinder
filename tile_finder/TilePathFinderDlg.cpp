
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

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD) {}

void CAboutDlg::DoDataExchange(CDataExchange* pDX) {
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CTilePathFinderDlg 对话框

CBrush* CTilePathFinderDlg::pBrushR = new CBrush(RGB(255, 0, 0));
CBrush* CTilePathFinderDlg::pBrushG = new CBrush(RGB(0, 255, 0));
CBrush* CTilePathFinderDlg::pBrushB = new CBrush(RGB(0, 0, 255));
CBrush* CTilePathFinderDlg::pBrushGray = new CBrush(RGB(0xcd, 0x66, 0x1d));
CBrush* CTilePathFinderDlg::pBrushDump = new CBrush(RGB(0, 0, 0));
CBrush* CTilePathFinderDlg::pBrushLine = new CBrush(RGB(124, 252, 0));
CBrush* CTilePathFinderDlg::pBrushStop = new CBrush(RGB(0, 255, 255));
CPen* CTilePathFinderDlg::pPenLine = new CPen(PS_SOLID, 1, RGB(110, 139, 61));
CPen* CTilePathFinderDlg::pPenLine1 = new CPen(PS_SOLID, 1, RGB(0, 139, 61));
CPen* CTilePathFinderDlg::pPenLine2 = new CPen(PS_SOLID, 1, RGB(110, 0, 61));


CTilePathFinderDlg::CTilePathFinderDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CTilePathFinderDlg::IDD, pParent) {
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CTilePathFinderDlg::DoDataExchange(CDataExchange* pDX) {
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CTilePathFinderDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CTilePathFinderDlg::OnFindPath)
	ON_BN_CLICKED(IDC_BUTTON2, &CTilePathFinderDlg::OnStraightLine)
	ON_EN_CHANGE(IDC_EDIT1, &CTilePathFinderDlg::OnChangeX)
	ON_EN_CHANGE(IDC_EDIT2, &CTilePathFinderDlg::OnChangeZ)
	ON_EN_CHANGE(IDC_EDIT3, &CTilePathFinderDlg::OnChangeEditBrush)
	//	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONUP()
	ON_UPDATE_COMMAND_UI(IDD_TILE_FINDPATH_DIALOG, &CTilePathFinderDlg::OnUpdateIddTileFindpathDialog)
	ON_EN_CHANGE(IDC_EDIT4, &CTilePathFinderDlg::OnEnChangeEdit4)
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_CHECK1, &CTilePathFinderDlg::OnBnClickedPathCheck)
	ON_BN_CLICKED(IDC_CHECK2, &CTilePathFinderDlg::OnBnClickedLineCheck)
	//ON_WM_LBUTTONDOWN()
	ON_BN_CLICKED(IDC_BUTTON3, &CTilePathFinderDlg::OnStraightLineEx)
	ON_BN_CLICKED(IDC_BUTTON4, &CTilePathFinderDlg::OnRandomPos)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_RBUTTONDOWN()
	ON_WM_MOUSEWHEEL()
	ON_WM_ERASEBKGND()
	ON_BN_CLICKED(IDC_CHECK3, &CTilePathFinderDlg::OnBnClickedCheck3)
	ON_BN_CLICKED(IDC_CHECK4, &CTilePathFinderDlg::OnBnClickedCheck4)
END_MESSAGE_MAP()


// CTilePathFinderDlg 消息处理程序

BOOL CTilePathFinderDlg::OnInitDialog() {
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

	m_offset_x = 0;
	m_offset_z = 0;
	m_scale = 0;


	m_begin_x = m_begin_z = -1;
	m_over_x = m_over_z = -1;

	CString tile_file;
	tile_file.Format(_T("./tile/%s"), AfxGetApp()->m_lpCmdLine);

	m_show_path_search = false;
	m_show_line_search = false;
	m_smooth_head = m_smooth_tail = false;
	bNeedPaint = true;

	USES_CONVERSION;
	char* pFileName = T2A(tile_file);
	m_tile_finder = TilePathFinder::LoadFromFile(pFileName);

	CRect rect;
	GetClientRect(&rect);
	m_cdc.resize(16);
	for (int i = 2; i < 16; i++) {
		m_scale = i;
		CDC* memdc = new CDC();
		m_cdc[i] = memdc;

		memdc->CreateCompatibleDC(GetDC());

		CBitmap memBitmap;
		memBitmap.CreateCompatibleBitmap(GetDC(), rect.Width(), rect.Height());
		memdc->SelectObject(&memBitmap);
		memdc->FillSolidRect(0, 0, rect.Width(), rect.Height(), RGB(255, 255, 255));

		CPen pen(PS_SOLID, 1, RGB(0, 0, 0));
		CPen* oriOpen = memdc->SelectObject(&pen);

		for (int x = 0; x < m_tile_finder->GetWidth(); x++) {
			for (int z = 0; z < m_tile_finder->GetHeight(); z++) {

				CBrush* oriBush = NULL;

				if (m_tile_finder->GetBlock(x, z) == 0 || m_tile_finder->GetBlock(x, z) == 1)
					oriBush = memdc->SelectObject(pBrushB);
				else
					oriBush = memdc->SelectObject(pBrushR);

				if (oriBush) {
					DrawTile(memdc, x, z);
					memdc->SelectObject(oriBush);
				}
			}
		}
		memdc->SelectObject(oriOpen);
	}
	m_scale = 5;
	m_offset_x = 200;
	m_offset_z = 10;
	m_cost = 50;
	m_brush_size = 2;

	CString str;
	str.Format(_T("%d"), m_offset_x);
	((CEdit*)GetDlgItem(IDC_EDIT1))->SetWindowTextW(str);
	str.Format(_T("%d"), m_offset_z);
	((CEdit*)GetDlgItem(IDC_EDIT2))->SetWindowTextW(str);
	str.Format(_T("%d"), m_brush_size);
	((CEdit*)GetDlgItem(IDC_EDIT3))->SetWindowTextW(str);
	str.Format(_T("%f"), m_cost);
	((CEdit*)GetDlgItem(IDC_EDIT4))->SetWindowTextW(str);

	((CButton*)GetDlgItem(IDC_CHECK1))->SetCheck(m_show_path_search);
	((CButton*)GetDlgItem(IDC_CHECK2))->SetCheck(m_show_line_search);
	((CButton*)GetDlgItem(IDC_CHECK3))->SetCheck(m_smooth_head);
	((CButton*)GetDlgItem(IDC_CHECK4))->SetCheck(m_smooth_tail);

	m_time_cost = new CStatic();
	m_time_cost->Create(_T(""), WS_CHILD | WS_VISIBLE | SS_LEFT, CRect(10, 20, 150, 100), this);

	m_pos_start = new CStatic();
	m_pos_start->Create(_T(""), WS_CHILD | WS_VISIBLE | SS_LEFT, CRect(10, 50, 150, 100), this);

	m_pos_over = new CStatic();
	m_pos_over->Create(_T(""), WS_CHILD | WS_VISIBLE | SS_LEFT, CRect(10, 80, 150, 100), this);

	AllocConsole();
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	int hCrt = _open_osfhandle((long)handle, _O_TEXT);
	FILE* hf = _fdopen(hCrt, "w");
	*stdout = *hf;

	Invalidate();
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CTilePathFinderDlg::OnSysCommand(UINT nID, LPARAM lParam) {
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

bool bPaint = false;
void CTilePathFinderDlg::OnPaint() {
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
		if (bNeedPaint) {
			bNeedPaint = false;
			UpdateDialog();
		}
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CTilePathFinderDlg::OnQueryDragIcon() {
	return static_cast<HCURSOR>(m_hIcon);
}

void CTilePathFinderDlg::FindPath(bool check_close) {
	m_path.clear();

	LARGE_INTEGER freq;
	LARGE_INTEGER start, over;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&start);

	m_tile_finder->SetDebugCallback(m_show_path_search ? CTilePathFinderDlg::OnSearchDump : NULL, this);

	int st = TilePathFinder::None;
	if (m_smooth_head) {
		st |= TilePathFinder::Head;
	}
	if (m_smooth_tail) {
		st |= TilePathFinder::Tail;
	}
	Math::Vector2 from((float)m_begin_x, (float)m_begin_z);
	Math::Vector2 to((float)m_over_x, (float)m_over_z);

	std::vector<const Math::Vector2*> list;
	m_tile_finder->Find(from, to, list, (TilePathFinder::SmoothType)st, check_close, m_cost);
	for (unsigned int i = 0; i < list.size(); i++) {
		POINT* pt = new POINT();
		pt->x = list[i]->x();
		pt->y = list[i]->y();
		m_path.push_back(pt);
	}

	QueryPerformanceCounter(&over);

	CClientDC cdc(this);
	CPen* oriPen = cdc.SelectObject(pPenLine);
	if (m_smooth_head) {
		oriPen = cdc.SelectObject(pPenLine1);
	}
	if (m_smooth_tail) {
		oriPen = cdc.SelectObject(pPenLine2);
	}

	if (m_path.size() != 0) {
		std::vector<POINT*>::iterator iter = m_path.begin();
		POINT pt;
		pt.x = ((*iter)->x + 0.5) * m_scale + m_offset_x;
		pt.y = ((*iter)->y + 0.5) * m_scale + m_offset_z;
		cdc.MoveTo(pt);

		for (iter++; iter != m_path.end(); iter++) {
			pt.x = ((*iter)->x + 0.5) * m_scale + m_offset_x;
			pt.y = ((*iter)->y + 0.5) * m_scale + m_offset_z;

			cdc.LineTo(pt);
			cdc.MoveTo(pt);
		}
	}

	cdc.SelectObject(oriPen);

	double pathCost = (double)((over.QuadPart - start.QuadPart) * 1000) / (double)freq.QuadPart;
	CString str;
	str.Format(_T("耗时:%fms"), pathCost);
	m_time_cost->SetWindowText(str);

	str.Format(_T("起点:%d,%d"), m_begin_x, m_begin_z);
	m_pos_start->SetWindowText(str);

	str.Format(_T("终点:%d,%d"), m_over_x, m_over_z);
	m_pos_over->SetWindowText(str);
}

void CTilePathFinderDlg::OnFindPath() {
	// TODO: 在此添加控件通知处理程序代码

	FindPath(false);
}

void CTilePathFinderDlg::OnChangeX() {
	CString str;
	((CEdit*)GetDlgItem(IDC_EDIT1))->GetWindowTextW(str);
	m_offset_x = _ttoi(str);
	bNeedPaint = true;
	Invalidate();
}

void CTilePathFinderDlg::OnChangeZ() {
	CString str;
	((CEdit*)GetDlgItem(IDC_EDIT2))->GetWindowTextW(str);
	m_offset_z = _ttoi(str);
	bNeedPaint = true;
	Invalidate();
}


void CTilePathFinderDlg::OnChangeEditBrush() {
	CString str;
	((CEdit*)GetDlgItem(IDC_EDIT3))->GetWindowTextW(str);
	m_brush_size = _ttoi(str);
	bNeedPaint = true;
	Invalidate();
}

bool CTilePathFinderDlg::Between(CPoint& pos) {
	if (pos.x >= m_offset_x && pos.y >= m_offset_z) {
		if (pos.x <= (m_tile_finder->GetWidth() - 1) * m_scale + m_offset_x && pos.y <= (m_tile_finder->GetHeight() - 1) * m_scale + m_offset_z) {
			return true;
		}
	}
	return false;
}

void CTilePathFinderDlg::UpdateDialog() {
	CRect rect;
	GetClientRect(&rect);
	CDC* memdc = m_cdc[m_scale];
	CDC* cdc = GetDC();
	cdc->BitBlt(m_offset_x, m_offset_z, rect.Width(), rect.Height(), memdc, 0, 0, SRCCOPY);
	DrawBegin();
	DrawOver();

	for (int x = 0; x < m_tile_finder->GetWidth(); x++) {
		for (int z = 0; z < m_tile_finder->GetHeight(); z++) {

			if (m_tile_finder->GetBlock(x, z) == 1) {
				CBrush* oriBush = cdc->SelectObject(pBrushGray);
				DrawTile(cdc, x, z);

				cdc->SelectObject(oriBush);
			}
		}
	}
}


void CTilePathFinderDlg::OnLButtonUp(UINT nFlags, CPoint point) {
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	m_drag_l = false;
	if (Between(point)) {
		BOOL bCtrl = GetKeyState(VK_CONTROL) & 0x8000;
		BOOL bAlt = GetKeyState(VK_MENU) & 0x8000;
		if (bCtrl && bAlt) {
			CClientDC dc(this);
			//m_tile_finder->SetDebugCallback(CTilePathFinderDlg::OnSearchDump, this);
			for (int i = 0; i < 1000; i++) {
				Math::Vector2 result;
				m_begin_x = (point.x - m_offset_x) / m_scale;
				m_begin_z = (point.y - m_offset_z) / m_scale;
				m_tile_finder->RandomInCircle(m_begin_x, m_begin_z, 10, result);
				dc.SetPixel((result[0] + (Math::Rand((float)0, (float)1))) * (float)m_scale + m_offset_x, (result[1] + (Math::Rand((float)0, (float)1))) * (float)m_scale + m_offset_z, RGB(255, 255, 250));
			}
		} else if (bCtrl) {
			CClientDC cdc(this);
			CBrush* oriBrush = cdc.SelectObject(pBrushGray);
			DrawBlock(&cdc, point, 1);
			cdc.SelectObject(oriBrush);
		} else {
			m_begin_x = (point.x - m_offset_x) / m_scale;
			m_begin_z = (point.y - m_offset_z) / m_scale;

			DrawBegin();
		}
	}

	CDialogEx::OnLButtonUp(nFlags, point);
}


void CTilePathFinderDlg::OnRButtonUp(UINT nFlags, CPoint point) {
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	m_drag_r = false;
	if (Between(point)) {
		BOOL bCtrl = GetKeyState(VK_CONTROL) & 0x8000;
		if (bCtrl) {
			CClientDC cdc(this);
			CBrush* oriBrush = cdc.SelectObject(pBrushB);
			DrawBlock(&cdc, point, 0);
			cdc.SelectObject(oriBrush);
		} else {
			m_over_x = (point.x - m_offset_x) / m_scale;
			m_over_z = (point.y - m_offset_z) / m_scale;

			if (m_tile_finder->IsBlock(m_over_x, m_over_z)) {
				m_tile_finder->SetDebugCallback(CTilePathFinderDlg::OnSearchDump, this);
				TilePathFinder::PathNode* node = m_tile_finder->SearchInCircle(m_over_x, m_over_z, TilePathFinder::kSearchDepth);
				if (!node) {
					node = m_tile_finder->SearchInReactangle(m_over_x, m_over_z, 16);
				}
				if (node) {
					m_over_x = node->pos_[0];
					m_over_z = node->pos_[1];
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


void CTilePathFinderDlg::OnUpdateIddTileFindpathDialog(CCmdUI* pCmdUI) {
	// TODO: 在此添加命令更新用户界面处理程序代码
	UpdateDialog();
}


void CTilePathFinderDlg::OnEnChangeEdit4() {
	CString str;
	((CEdit*)GetDlgItem(IDC_EDIT4))->GetWindowTextW(str);
	m_cost = _ttoi(str);
}


void CTilePathFinderDlg::OnClose() {
	CString tile_file;
	tile_file.Format(_T("./tile/%s"), AfxGetApp()->m_lpCmdLine);
	USES_CONVERSION;
	char* pFileName = T2A(tile_file);
	m_tile_finder->Serialize(pFileName);
	CDialogEx::OnClose();
}


void CTilePathFinderDlg::OnBnClickedPathCheck() {
	m_show_path_search = ((CButton*)GetDlgItem(IDC_CHECK1))->GetCheck();
}


void CTilePathFinderDlg::OnBnClickedLineCheck() {
	// TODO:  在此添加控件通知处理程序代码
	m_show_line_search = ((CButton*)GetDlgItem(IDC_CHECK2))->GetCheck();
}


void CTilePathFinderDlg::OnStraightLine() {
	// TODO: 在此添加控件通知处理程序代码
	//RayCast(0);
	FindPath(true);
}


void CTilePathFinderDlg::OnStraightLineEx() {
	// TODO:  在此添加控件通知处理程序代码
	//RayCast(1);
	FindPath(false);
}

void CTilePathFinderDlg::RayCast(int type) {
	int rx = 0;
	int ry = 0;

	LARGE_INTEGER freq;
	LARGE_INTEGER start, over;
	QueryPerformanceFrequency(&freq);
	QueryPerformanceCounter(&start);

	m_tile_finder->SetDebugCallback(m_show_line_search ? CTilePathFinderDlg::OnSearchDump : NULL, this);

	const Math::Vector2 fromPos((float)m_begin_x, (float)m_begin_z);
	const Math::Vector2 toPos((float)m_over_x, (float)m_over_z);
	Math::Vector2 result;
	Math::Vector2 stop;

	if (type == 0) {
		m_tile_finder->Raycast(fromPos, toPos, false, &result, &stop, true);
	} else {
		m_tile_finder->Raycast(fromPos, toPos, false, &result, &stop, false);
	}

	QueryPerformanceCounter(&over);

	double pathCost = (double)((over.QuadPart - start.QuadPart) * 1000) / (double)freq.QuadPart;


	CClientDC cdc(this);
	CPen* oriPen = cdc.SelectObject(pPenLine);
	POINT from;
	from.x = (m_begin_x + 0.5) * m_scale + m_offset_x;
	from.y = (m_begin_z + 0.5) * m_scale + m_offset_z;
	POINT to;
	to.x = (result[0] + 0.5) * m_scale + m_offset_x;
	to.y = (result[1] + 0.5) * m_scale + m_offset_z;
	cdc.MoveTo(from);
	cdc.LineTo(to);
	cdc.SelectObject(oriPen);

	CBrush* ori = cdc.SelectObject(pBrushStop);
	DrawTile(&cdc, stop[0], stop[1]);
	cdc.SelectObject(ori);

	CString str;
	str.Format(_T("耗时:%fms"), pathCost);
	m_time_cost->SetWindowText(str);

	str.Format(_T("起点:%d,%d"), m_begin_x, m_begin_z);
	m_pos_start->SetWindowText(str);

	str.Format(_T("终点:%d,%d"), m_over_x, m_over_z);
	m_pos_over->SetWindowText(str);
}

void CTilePathFinderDlg::OnRandomPos() {
	// TODO:  在此添加控件通知处理程序代码
	CClientDC cdc(this);

	for (int i = 0; i < 10000; i++) {
		Math::Vector2 result;
		m_tile_finder->Random(result);
		POINT pt;
		pt.x = ((float)result[0] + Math::Rand(0.0f, 1.0f)) * (float)m_scale + (float)m_offset_x;
		pt.y = ((float)result[1] + Math::Rand(0.0f, 1.0f)) * (float)m_scale + (float)m_offset_z;

		cdc.Ellipse(pt.x - 2, pt.y - 2, pt.x + 2, pt.y + 2);
	}
}

void CTilePathFinderDlg::DrawTile(CDC* cdc, int x, int z) {
	CPoint pt[4];
	pt[0].x = x * m_scale + m_offset_x;
	pt[0].y = z * m_scale + m_offset_z;
	pt[1].x = (x + 1) * m_scale + m_offset_x;
	pt[1].y = z * m_scale + m_offset_z;
	pt[2].x = (x + 1) * m_scale + m_offset_x;
	pt[2].y = (z + 1) * m_scale + m_offset_z;
	pt[3].x = x * m_scale + m_offset_x;
	pt[3].y = (z + 1) * m_scale + m_offset_z;

	bool ok = cdc->Polygon(pt, 4);
}

void CTilePathFinderDlg::DrawBlock(CDC* cdc, CPoint& pos, uint8_t val) {
	int x = (pos.x - m_offset_x) / m_scale;
	int z = (pos.y - m_offset_z) / m_scale;

	for (int i = 0; i < m_brush_size; i++) {
		for (int j = 0; j < m_brush_size; j++) {
			uint8_t block = m_tile_finder->GetBlock(x + i, z + j);
			if (block == 0 || block == 1) {
				m_tile_finder->SetBlock(x + i, z + j, val);
				DrawTile(cdc, x + i, z + j);
			}
		}
	}
}

void CTilePathFinderDlg::DrawBegin() {
	CClientDC dc(this);

	CString str;
	str.Format(_T("起点:%d,%d"), -1, -1);
	m_pos_start->SetWindowText(str);

	CBrush brush_begin(RGB(0, 255, 0));
	CBrush* obrush = dc.SelectObject(&brush_begin);
	if (m_begin_x != -1 && m_begin_z != -1) {
		DrawTile(&dc, m_begin_x, m_begin_z);
		str.Format(_T("起点:%d,%d"), m_begin_x, m_begin_z);
		m_pos_start->SetWindowText(str);
	}
	dc.SelectObject(obrush);
}

void CTilePathFinderDlg::DrawOver() {
	CClientDC dc(this);

	CString str;
	str.Format(_T("终点:%d,%d"), -1, -1);
	m_pos_over->SetWindowText(str);

	CBrush brush_over(RGB(0, 0, 0));
	CBrush* obrush = dc.SelectObject(&brush_over);
	if (m_over_x != -1 && m_over_z != -1) {
		DrawTile(&dc, m_over_x, m_over_z);
		str.Format(_T("终点:%d,%d"), m_over_x, m_over_z);
		m_pos_over->SetWindowText(str);
	}
	dc.SelectObject(obrush);
}

void CTilePathFinderDlg::OnMouseMove(UINT nFlags, CPoint point) {
	// TODO:  在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnMouseMove(nFlags, point);
	BOOL bCtrl = GetKeyState(VK_CONTROL) & 0x8000;
	if (bCtrl) {
		CClientDC cdc(this);
		if (m_drag_l) {
			CBrush* oriBrush = cdc.SelectObject(pBrushGray);
			DrawBlock(&cdc, point, 1);
			cdc.SelectObject(oriBrush);
		}
		if (m_drag_r) {
			CBrush* oriBrush = cdc.SelectObject(pBrushB);
			DrawBlock(&cdc, point, 0);
			cdc.SelectObject(oriBrush);
		}
	}
}

void CTilePathFinderDlg::OnLButtonDown(UINT nFlags, CPoint point) {
	// TODO:  在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnLButtonDown(nFlags, point);
	m_drag_l = true;
}

void CTilePathFinderDlg::OnRButtonDown(UINT nFlags, CPoint point) {
	// TODO:  在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnRButtonDown(nFlags, point);
	m_drag_r = true;
}

BOOL CTilePathFinderDlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) {
	// TODO: Add your message handler code here and/or call default
	if (zDelta > 0) {
		m_scale += 1;
	} else {
		m_scale -= 1;
	}
	if (m_scale <= 1 || m_scale >= 16) {
		if (m_scale <= 1) {
			m_scale = 2;
		}
		if (m_scale >= 16) {
			m_scale = 15;
		}
		return CDialogEx::OnMouseWheel(nFlags, zDelta, pt);
	}
	bNeedPaint = true;
	Invalidate();
	return CDialogEx::OnMouseWheel(nFlags, zDelta, pt);
}

void CTilePathFinderDlg::OnSearchDump(void* userdata, int x, int z) {
	CTilePathFinderDlg* ptr = (CTilePathFinderDlg*)userdata;
	CClientDC cdc(ptr);
	CBrush* oriBrush = cdc.SelectObject(pBrushDump);

	ptr->DrawTile(&cdc, x, z);
	cdc.SelectObject(oriBrush);
	//Sleep(1);
}


BOOL CTilePathFinderDlg::OnEraseBkgnd(CDC* pDC) {
	// TODO: Add your message handler code here and/or call default
	if (bNeedPaint) {
		return CDialogEx::OnEraseBkgnd(pDC);
	}
	return true;
}


void CTilePathFinderDlg::OnBnClickedCheck3() {
	// TODO: Add your control notification handler code here
	m_smooth_head = ((CButton*)GetDlgItem(IDC_CHECK3))->GetCheck();
}


void CTilePathFinderDlg::OnBnClickedCheck4() {
	// TODO: Add your control notification handler code here
	m_smooth_tail = ((CButton*)GetDlgItem(IDC_CHECK4))->GetCheck();
}
