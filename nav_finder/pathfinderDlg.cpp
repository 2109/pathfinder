
// PathFinderTestDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "pathfinder.h"
#include "pathfinderDlg.h"
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
	enum {
		IDD = IDD_ABOUTBOX
	};

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


// CNavDlg 对话框



CNavDlg::CNavDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CNavDlg::IDD, pParent) {
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

}

void CNavDlg::DoDataExchange(CDataExchange* pDX) {
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CNavDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &CNavDlg::OnPath)
	ON_WM_MOUSEACTIVATE()
	ON_UPDATE_COMMAND_UI(IDD_PATHFINDERTEST_DIALOG, &CNavDlg::OnUpdateIddPathfindertestDialog)
	ON_BN_CLICKED(IDC_BUTTON2, &CNavDlg::Straightline)
	ON_WM_LBUTTONUP()
	ON_BN_CLICKED(IDC_BUTTON3, &CNavDlg::OnIgnoreLine)
	ON_EN_CHANGE(IDC_EDIT2, &CNavDlg::OnEnChangeEdit2)
	ON_EN_CHANGE(IDC_EDIT3, &CNavDlg::OnEnChangeEdit3)
	ON_EN_CHANGE(IDC_EDIT4, &CNavDlg::OnEnChangeEdit4)
	ON_BN_CLICKED(IDC_BUTTON4, &CNavDlg::OnIgnorePath)
	ON_WM_RBUTTONUP()
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_CHECK1, &CNavDlg::OnCheck)
	ON_EN_CHANGE(IDC_EDIT5, &CNavDlg::OnEnChangeEdit5)
	ON_EN_CHANGE(IDC_EDIT6, &CNavDlg::OnEnChangeEdit6)
	ON_WM_MOUSEWHEEL()
	ON_WM_MBUTTONUP()
	ON_WM_MBUTTONDOWN()
	ON_WM_MOUSEMOVE()
END_MESSAGE_MAP()


// CNavDlg 消息处理程序

BOOL CNavDlg::OnInitDialog() {
	CDialogEx::OnInitDialog();
	//_CrtSetBreakAlloc(3850);
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

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO:  在此添加额外的初始化代码

	CString nav_tile;
	nav_tile.Format(_T("./nav/%s"), AfxGetApp()->m_lpCmdLine);
	USES_CONVERSION;
	m_finder = NavPathFinder::LoadMeshEx(T2A(nav_tile.GetBuffer(0)));
	m_finder->CreateTile(100);

	m_mouse_state = true;
	m_mouse_point = NULL;

	m_poly_begin = -1;
	m_poly_over = -1;

	m_pt_over = m_pt_begin = NULL;

	CRect rect;
	GetClientRect(&rect);
	m_cdc.resize(20);

	m_offset_x = 0;
	m_offset_z = 0;
	CBrush brushWalk(RGB(255, 0, 0));
	CBrush brushBlock(RGB(88, 88, 0));
	for (int i = 0; i < 20;i++) {
		m_scale = 0.005 * i + 0.005;
		CDC* memdc = new CDC();
		m_cdc[i] = memdc;

		memdc->CreateCompatibleDC(GetDC());

		CBitmap memBitmap;
		memBitmap.CreateCompatibleBitmap(GetDC(), rect.Width(), rect.Height());
		memdc->SelectObject(&memBitmap);
		memdc->FillSolidRect(0, 0, rect.Width(), rect.Height(), RGB(255, 255, 255));

		for (int i = 0; i < m_finder->mesh_->node_.size(); i++) {
			NavNode* node = m_finder->GetNode(i);
			CPoint* pt = new CPoint[node->size_];

			CBrush* ori = NULL;
			if (node->mask_ == 0) {
				ori = memdc->SelectObject(&brushWalk);
			} else {
				ori = memdc->SelectObject(&brushBlock);
			}
			for (int j = 0; j < node->size_; j++) {
				Math::Vector3* pos = &m_finder->mesh_->vertice_[node->vertice_[j]];

				pt[j].x = pos->x*m_scale + m_offset_x;
				pt[j].y = pos->z*m_scale + m_offset_z;
			}
			memdc->Polygon(pt, node->size_);
			delete[] pt;
			memdc->SelectObject(ori);
		}
	}

	m_scale_base = 10;
	m_scale = 0.005f + (m_scale_base * 0.005);
	m_offset_x = 200;
	m_offset_z = 0;

	CString str;
	str.Format(_T("%d"), m_offset_x);
	((CEdit*)GetDlgItem(IDC_EDIT2))->SetWindowTextW(str);
	str.Format(_T("%d"), m_offset_z);
	((CEdit*)GetDlgItem(IDC_EDIT3))->SetWindowTextW(str);
	str.Format(_T("%f"), m_scale);
	((CEdit*)GetDlgItem(IDC_EDIT4))->SetWindowTextW(str);

	AllocConsole();
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	int hCrt = _open_osfhandle((long)handle, _O_TEXT);
	FILE * hf = _fdopen(hCrt, "w");
	*stdout = *hf;

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CNavDlg::OnSysCommand(UINT nID, LPARAM lParam) {
	if ((nID & 0xFFF0) == IDM_ABOUTBOX) {
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	} else {
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CNavDlg::OnPaint() {
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

	DrawMap();
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CNavDlg::OnQueryDragIcon() {
	return static_cast<HCURSOR>(m_hIcon);
}

void OnSearchDump(void* self, int index) {
	CNavDlg* dlgPtr = (CNavDlg*)self;
	CClientDC dc(dlgPtr);
	CBrush brush(RGB(123, 255, 0));
	dc.SelectObject(&brush);

	NavNode* node = dlgPtr->m_finder->GetNode(index);
	CPoint* pt = new CPoint[node->size_];
	for (int j = 0; j < node->size_; j++) {
		Math::Vector3* pos = &dlgPtr->m_finder->mesh_->vertice_[node->vertice_[j]];
		pt[j].x = pos->x*dlgPtr->m_scale + dlgPtr->m_offset_x;
		pt[j].y = pos->z*dlgPtr->m_scale + dlgPtr->m_offset_z;
	}
	dc.Polygon(pt, node->size_);
	delete[] pt;
	//Sleep(5);
}

void OnOverlayDump(void* self, std::vector<const Math::Vector3*>& poly) {
	CNavDlg* dlgPtr = (CNavDlg*)self;
	CClientDC dc(dlgPtr);
	CBrush brush(RGB(123, 255, 0));
	dc.SelectObject(&brush);

	CPoint* pt = new CPoint[poly.size()];
	for (int j = 0; j < poly.size(); j++) {
		const Math::Vector3* pos = poly[j];
		pt[j].x = pos->x*dlgPtr->m_scale + dlgPtr->m_offset_x;
		pt[j].y = pos->z*dlgPtr->m_scale + dlgPtr->m_offset_z;
	}
	dc.Polygon(pt, poly.size());
	delete[] pt;
	//Sleep(5);
}

void CNavDlg::OnPath() {
	// TODO:  在此添加控件通知处理程序代码

	if (m_poly_begin == -1) {
		CString str;
		str.Format(_T("始点还没设置"));
		MessageBox(str);
	} else if (m_poly_over == -1) {
		CString str;
		str.Format(_T("终点还没设置"));
		MessageBox(str);
	} else {
		for (int i = 0; i < 8; i++) {
			m_finder->SetMask(i, 1);
		}
		m_finder->SetMask(1, 0);

		Math::Vector3 begin((m_pt_begin->x - m_offset_x) / m_scale, 0, (m_pt_begin->z - m_offset_z) / m_scale);
		Math::Vector3 over((m_pt_over->x - m_offset_x) / m_scale, 0, (m_pt_over->z - m_offset_z) / m_scale);

		std::vector<const Math::Vector3*> result;
		if (m_finder->Find(begin, over, result) >= 0) {
			DrawPath(result);
		}
	}
}


int CNavDlg::OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message) {
	// TODO:  在此添加消息处理程序代码和/或调用默认值

	return CDialogEx::OnMouseActivate(pDesktopWnd, nHitTest, message);
}


void CNavDlg::DrawMap() {
	CRect rect;
	GetClientRect(&rect);
	
	CDC* memdc = m_cdc[m_scale_base];

	CDC* cdc = GetDC();
	cdc->BitBlt(m_offset_x, m_offset_z, rect.Width(), rect.Height(), memdc, 0, 0, SRCCOPY);

	CPen pen(PS_SOLID, 1, RGB(0, 0, 0));
	CPen *oriPen = cdc->SelectObject(&pen);

	CBrush brush(RGB(255, 0, 0));
	CBrush *obrush = cdc->SelectObject(&brush);

	CBrush brush_empty0(RGB(255, 255, 0));
	CBrush brush_empty1(RGB(0, 0, 0));
	CBrush brush_empty2(RGB(88, 88, 0));

	int check = ((CButton *)GetDlgItem(IDC_CHECK1))->GetCheck();
	if (check) {
		for (int i = 0; i < m_finder->mesh_->tileWidth_ * m_finder->mesh_->tileHeight_; i++) {
			NavTile* tile = &m_finder->mesh_->tile_[i];
			CPoint pt[4];

			if (tile->node_.size() == 0)
				cdc->SelectObject(&brush_empty0);
			else
				cdc->SelectObject(&brush_empty1);


			for (int j = 0; j < 4; j++) {
				Math::Vector3* pos = &tile->pos_[j];

				pt[j].x = pos->x*m_scale + m_offset_x;
				pt[j].y = pos->z*m_scale + m_offset_z;
			}
			cdc->Polygon(pt, 4);
		}
	}

	cdc->SelectObject(obrush);

	if (m_poly_begin != -1) {
		CBrush brush(RGB(0, 255, 0));
		cdc->SelectObject(&brush);
		NavNode* node = m_finder->GetNode(m_poly_begin);
		CPoint* pt = new CPoint[node->size_];
		for (int j = 0; j < node->size_; j++) {
			Math::Vector3* pos = &m_finder->mesh_->vertice_[node->vertice_[j]];
			pt[j].x = pos->x*m_scale + m_offset_x;
			pt[j].y = pos->z*m_scale + m_offset_z;
		}
		cdc->Polygon(pt, node->size_);
		delete[] pt;
	}

	if (m_poly_over != -1) {
		CBrush brush(RGB(0, 0, 255));
		cdc->SelectObject(&brush);

		NavNode* node = m_finder->GetNode(m_poly_over);
		CPoint* pt = new CPoint[node->size_];
		for (int j = 0; j < node->size_; j++) {
			Math::Vector3* pos = &m_finder->mesh_->vertice_[node->vertice_[j]];
			pt[j].x = pos->x*m_scale + m_offset_x;
			pt[j].y = pos->z*m_scale + m_offset_z;
		}
		cdc->Polygon(pt, node->size_);
		delete[] pt;
	}


	if (m_pt_begin != NULL) {
		CBrush brush(RGB(50, 50, 50));
		cdc->SelectObject(&brush);
		cdc->Ellipse(m_pt_begin->x - 3, m_pt_begin->z - 3, m_pt_begin->x + 3, m_pt_begin->z + 3);
	}

	if (m_pt_over != NULL) {
		CBrush brush(RGB(250, 50, 50));
		cdc->SelectObject(&brush);
		cdc->Ellipse(m_pt_over->x - 3, m_pt_over->z - 3, m_pt_over->x + 3, m_pt_over->z + 3);
	}

	cdc->SelectObject(oriPen);
	cdc->SelectObject(obrush);

}

void OnAroundDump(void* self, int index) {
	CNavDlg* dlgPtr = (CNavDlg*)self;
	CClientDC dc(dlgPtr);
	CBrush brush(RGB(66, 66, 66));

	NavTile* tile = &dlgPtr->m_finder->mesh_->tile_[index];
	CPoint pt[4];


	CBrush* obrush = dc.SelectObject(&brush);

	for (int j = 0; j < 4; j++) {
		Math::Vector3* pos = &tile->pos_[j];

		pt[j].x = pos->x*dlgPtr->m_scale + dlgPtr->m_offset_x;
		pt[j].y = pos->z*dlgPtr->m_scale + dlgPtr->m_offset_z;
	}
	dc.Polygon(pt, 4);
	dc.SelectObject(obrush);
}

void CNavDlg::DrawBegin(CPoint& pos) {
	double nav_x = (double)(pos.x - m_offset_x) / m_scale;
	double nav_z = (double)(pos.y - m_offset_z) / m_scale;

	printf("nav begin:%f,%f\n", nav_x, nav_z);

	/*m_finder->SetDebugTileFunc(OnAroundDump, this);
	m_finder->SetDebugNodeFunc(OnSearchDump, this);*/
	NavNode* node = m_finder->SearchNode(Math::Vector3(nav_x, 0, nav_z), 20);
	if (node == NULL)
		return;

	if (m_pt_begin != NULL) {
		free(m_pt_begin);
		m_pt_begin = NULL;
	}

	m_finder->Dot2Node(Math::Vector3(nav_x, 0, nav_z), node->id_);

	m_poly_begin = node->id_;

	m_pt_begin = new Math::Vector3();
	m_pt_begin->x = pos.x;
	m_pt_begin->z = pos.y;


	printf("nav node:%d\n", node->id_);
	for (int i = 0; i < node->size_; i++)
		printf("%d,%f\t", node->vertice_[i], m_finder->mesh_->vertice_[node->vertice_[i]].y);
	printf("\n");

	float h = m_finder->GetHeight(Math::Vector3(nav_x, 0, nav_z));
	printf("heigh:%f\n", h);

	//m_finder->SetDebugOverlapFunc(OnOverlayDump, this);

	//CClientDC dc(this);
	//for (int i = 0; i < 1; i++) {
	//	Math::Vector3 result;
	//	//Math::Vector3 result = m_finder->RandomMovable(-1);
	//	if (m_finder->RandomInCircle(result, Math::Vector3(nav_x, 0, nav_z), 2000)) {
	//		printf("%f,%f\n", result.x*m_scale + m_offset_x, result.z*m_scale + m_offset_z);
	//		dc.SetPixel(result.x*m_scale + m_offset_x, result.z*m_scale + m_offset_z, RGB(255, 111, 250));
	//	}

	//}

	Invalidate();
}



void CNavDlg::DrawOver(CPoint& pos) {
	double nav_x = (double)(pos.x - m_offset_x) / m_scale;
	double nav_z = (double)(pos.y - m_offset_z) / m_scale;

	NavNode* node = NULL;
	int node_index;
	float offset;

	if (!m_finder->Movable(Math::Vector3(nav_x, 0, nav_z), 10, &offset)) {
		printf("offset:%f\n", offset);

		m_finder->SetDebugTileFunc(OnAroundDump, this);

		int nodeIndex = 0;
		Math::Vector3* vt = m_finder->SearchInCircle(Math::Vector3(nav_x, 0, nav_z), 5, &nodeIndex);
		if (!vt) {
			printf("%f,%f\n", nav_x, nav_z);
			return;
		}


		node = m_finder->GetNode(nodeIndex);

		CBrush brush(RGB(66, 88, 188));
		CClientDC dc(this);
		CBrush* obrush = dc.SelectObject(&brush);
		dc.Ellipse(vt->x*m_scale + m_offset_x - 3, vt->z*m_scale + m_offset_z - 3, vt->x*m_scale + m_offset_x + 3, vt->z*m_scale + m_offset_z + 3);
		dc.SelectObject(obrush);
		pos.x = vt->x*m_scale + m_offset_x;
		pos.y = vt->z*m_scale + m_offset_z;

		if (m_pt_over != NULL) {
			free(m_pt_over);
			m_pt_over = NULL;
		}

		m_pt_over = new Math::Vector3();
		m_pt_over->x = vt->x*m_scale + m_offset_x;
		m_pt_over->z = vt->z*m_scale + m_offset_z;

		printf("nav over:%f,%f\n", vt->x, vt->z);
		printf("nav node:%d\n", node->id_);

		//vector3 start;
		//start.x = nav_x;
		//start.z = nav_z;
		//vector3 result;
		//bool ok = raycast(m_mesh, vt, &start, &result, NULL, this);
		//if ( ok ){
		//	dc.Ellipse(result.x*m_scale + m_offset_x - 3, result.z*m_scale + m_offset_z - 3, result.x*m_scale + m_offset_x + 3, result.z*m_scale + m_offset_z + 3);
		//	dc.SelectObject(obrush);
		//}
	} else {
		node = m_finder->SearchNode(Math::Vector3(nav_x, 0, nav_z), 5);

		if (m_pt_over != NULL) {
			free(m_pt_over);
			m_pt_over = NULL;
		}

		m_pt_over = new Math::Vector3();
		m_pt_over->x = pos.x;
		m_pt_over->z = pos.y;

		printf("nav over:%f,%f\n", nav_x, nav_z);
		printf("nav node:%d\n", node->id_);

		Invalidate();
	}

	for (int i = 0; i < node->size_; i++)
		printf("%d\t", node->vertice_[i]);
	printf("\n");

	m_poly_over = node->id_;


}

void CNavDlg::OnUpdateIddPathfindertestDialog(CCmdUI *pCmdUI) {
	// TODO:  在此添加命令更新用户界面处理程序代码
	DrawMap();
}



void CNavDlg::Straightline() {
	// TODO:  在此添加控件通知处理程序代码
	if (m_poly_begin == -1) {
		CString str;
		str.Format(_T("始点还没设置"));
		MessageBox(str);
	} else if (m_poly_over == -1) {
		CString str;
		str.Format(_T("终点还没设置"));
		MessageBox(str);
	} else {
		for (int i = 0; i < 8; i++) {
			m_finder->SetMask(i, 1);
		}
		m_finder->SetMask(1, 0);


		Math::Vector3 begin((m_pt_begin->x - m_offset_x) / m_scale, 0, (m_pt_begin->z - m_offset_z) / m_scale);
		Math::Vector3 over((m_pt_over->x - m_offset_x) / m_scale, 0, (m_pt_over->z - m_offset_z) / m_scale);

		POINT from;
		from.x = begin.x*m_scale + m_offset_x;
		from.y = begin.z*m_scale + m_offset_z;

		CPen pen(PS_SOLID, 1, RGB(255, 255, 255));
		CClientDC dc(this);
		CPen *open = dc.SelectObject(&pen);
		dc.MoveTo(from);

		Math::Vector3 result;
		m_finder->Raycast(begin, over, result);

		POINT to;
		to.x = result.x*m_scale + m_offset_x;
		to.y = result.z*m_scale + m_offset_z;

		dc.LineTo(to);
		dc.SelectObject(open);
	}
}


void CNavDlg::OnLButtonDown(UINT nFlags, CPoint point) {
	// TODO:  在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnLButtonDown(nFlags, point);
}


void CNavDlg::OnLButtonUp(UINT nFlags, CPoint point) {
	// TODO:  在此添加消息处理程序代码和/或调用默认值


	this->DrawBegin(point);

	CDialogEx::OnLButtonUp(nFlags, point);
}



void CNavDlg::OnIgnoreLine() {
	// TODO:  在此添加控件通知处理程序代码
	if (m_poly_begin == -1) {
		CString str;
		str.Format(_T("始点还没设置"));
		MessageBox(str);
	} else if (m_poly_over == -1) {
		CString str;
		str.Format(_T("终点还没设置"));
		MessageBox(str);
	} else {
		for (int i = 0; i < 8; i++) {
			m_finder->SetMask(i, 1);
		}
		m_finder->SetMask(1, 0);

		m_finder->SetDebugNodeFunc(OnSearchDump, this);
		Math::Vector3 begin((m_pt_begin->x - m_offset_x) / m_scale, 0, (m_pt_begin->z - m_offset_z) / m_scale);
		Math::Vector3 over((m_pt_over->x - m_offset_x) / m_scale, 0, (m_pt_over->z - m_offset_z) / m_scale);

		POINT from;
		from.x = begin.x*m_scale + m_offset_x;
		from.y = begin.z*m_scale + m_offset_z;

		CPen pen(PS_SOLID, 1, RGB(255, 255, 255));
		CClientDC dc(this);
		CPen *open = dc.SelectObject(&pen);
		dc.MoveTo(from);

		Math::Vector3 result;
		m_finder->Raycast(begin, over, result);

		POINT to;
		to.x = result.x*m_scale + m_offset_x;
		to.y = result.z*m_scale + m_offset_z;

		dc.LineTo(to);
		dc.SelectObject(open);

		m_finder->SetDebugNodeFunc(NULL, NULL);
	}
}


void CNavDlg::OnEnChangeEdit2() {
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。



	// TODO:  在此添加控件通知处理程序代码
	CString str;
	((CEdit*)GetDlgItem(IDC_EDIT2))->GetWindowTextW(str);
	m_offset_x = _ttoi(str);
	Invalidate();
}


void CNavDlg::OnEnChangeEdit3() {
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。



	// TODO:  在此添加控件通知处理程序代码
	CString str;
	((CEdit*)GetDlgItem(IDC_EDIT3))->GetWindowTextW(str);
	m_offset_z = _ttoi(str);
	Invalidate();
}


void CNavDlg::OnEnChangeEdit4() {
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	CString str;
	((CEdit*)GetDlgItem(IDC_EDIT4))->GetWindowTextW(str);
	m_scale = _ttof(str);
	Invalidate();

	// TODO:  在此添加控件通知处理程序代码
}

void CNavDlg::DrawPath(std::vector<const Math::Vector3*>& path) {
	CClientDC dc(this);
	CPen pen(PS_SOLID, 1, RGB(255, 255, 255));
	CPen* open = dc.SelectObject(&pen);


	dc.MoveTo(path[0]->x*m_scale + m_offset_x, path[0]->z*m_scale + m_offset_z);
	for (int i = 1; i < path.size(); i++) {
		dc.LineTo(path[i]->x*m_scale + m_offset_x, path[i]->z*m_scale + m_offset_z);
		dc.MoveTo(path[i]->x*m_scale + m_offset_x, path[i]->z*m_scale + m_offset_z);
	}

	dc.SelectObject(open);
}

void CNavDlg::OnIgnorePath() {
	if (m_poly_begin == -1) {
		CString str;
		str.Format(_T("始点还没设置"));
		MessageBox(str);
	} else if (m_poly_over == -1) {
		CString str;
		str.Format(_T("终点还没设置"));
		MessageBox(str);
	} else {
		for (int i = 0; i < 8; i++) {
			m_finder->SetMask(i, 1);
		}

		m_finder->SetDebugNodeFunc(OnSearchDump, this);

		Math::Vector3 begin((m_pt_begin->x - m_offset_x) / m_scale, 0, (m_pt_begin->z - m_offset_z) / m_scale);
		Math::Vector3 over((m_pt_over->x - m_offset_x) / m_scale, 0, (m_pt_over->z - m_offset_z) / m_scale);

		std::vector<const Math::Vector3*> result;
		if (m_finder->Find(begin, over, result) >= 0) {
			DrawPath(result);
		}

		m_finder->SetDebugNodeFunc(NULL, NULL);
	}
}


void CNavDlg::OnRButtonUp(UINT nFlags, CPoint point) {
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	this->DrawOver(point);
	CDialogEx::OnRButtonUp(nFlags, point);
}


void CNavDlg::OnClose() {
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	CDialogEx::OnClose();
}

void CNavDlg::OnCheck() {
	// TODO:  在此添加控件通知处理程序代码
	Invalidate();
}

void CNavDlg::OnEnChangeEdit5() {
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码

}


void CNavDlg::OnEnChangeEdit6() {
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码

}


BOOL CNavDlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) {
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	if (zDelta > 0) {
		m_scale_base++;
	} else {
		m_scale_base--;
	}

	if (m_scale_base < 0 || m_scale_base > 19) {
		if (m_scale_base < 0) {
			m_scale_base = 0;
		}
		if (m_scale_base > 19) {
			m_scale_base = 19;
		}
	}
	m_scale = (0.005 + (m_scale_base * 0.005));

	printf("m_scale:%f, m_scale_base:%d\n", m_scale, m_scale_base);
	CString str;
	str.Format(_T("%f"), m_scale);

	((CEdit*)GetDlgItem(IDC_EDIT4))->SetWindowTextW(str);

	Invalidate();
	return CDialogEx::OnMouseWheel(nFlags, zDelta, pt);
}


void CNavDlg::OnMButtonUp(UINT nFlags, CPoint point) {
	// TODO:  在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnMButtonUp(nFlags, point);
	m_mouse_state = true;
	if (m_mouse_point) {
		delete m_mouse_point;
		m_mouse_point = NULL;
	}
}


void CNavDlg::OnMButtonDown(UINT nFlags, CPoint point) {
	// TODO:  在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnMButtonDown(nFlags, point);

	m_mouse_state = false;
	m_mouse_point = new CPoint(point);
}


void CNavDlg::OnMouseMove(UINT nFlags, CPoint point) {
	// TODO:  在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnMouseMove(nFlags, point);
	if (m_mouse_state == false) {
		m_offset_x = point.x - m_mouse_point->x;
		m_offset_z = point.y - m_mouse_point->y;

		CString str;
		str.Format(_T("%d"), m_offset_x);

		((CEdit*)GetDlgItem(IDC_EDIT2))->SetWindowTextW(str);

		str.Format(_T("%d"), m_offset_z);

		((CEdit*)GetDlgItem(IDC_EDIT3))->SetWindowTextW(str);

		Invalidate();
	}
}
