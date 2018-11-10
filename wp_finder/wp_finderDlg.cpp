
// wp_finderDlg.cpp : 实现文件
//

#include "stdafx.h"
#include "wp_finder.h"
#include "wp_finderDlg.h"
#include "afxdialogex.h"

#include <io.h>    
#include <fcntl.h>  
#include <fstream>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
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

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// Cwp_finderDlg 对话框



Cwp_finderDlg::Cwp_finderDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(Cwp_finderDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void Cwp_finderDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(Cwp_finderDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_WM_MBUTTONUP()
	ON_WM_MBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONUP()
	ON_BN_CLICKED(IDC_BUTTON1, &Cwp_finderDlg::OnBnClickedButton1)
END_MESSAGE_MAP()


// Cwp_finderDlg 消息处理程序

BOOL Cwp_finderDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
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
	FILE* fp = _wfopen(nav_tile.GetBuffer(0), _T("rb"));

	UINT32 vertex_count;
	fread(&vertex_count, sizeof( UINT32 ), 1, fp);

	double** v_ptr = (double**)malloc(sizeof( *v_ptr ) * vertex_count);
	for ( int i = 0; i < vertex_count; i++ )
	{
		v_ptr[i] = (double*)malloc(sizeof(double)* 3);
		for ( int j = 0; j < 3; j++ )
		{
			float val;
			fread(&val, sizeof( float ), 1, fp);
			v_ptr[i][j] = val;
		}
	}

	UINT32 poly_count;
	fread(&poly_count, sizeof( UINT32 ), 1, fp);

	int** p_ptr = (int**)malloc(sizeof( *p_ptr ) * poly_count);
	for ( int i = 0; i < poly_count; i++ )
	{
		UINT8 index_count;
		fread(&index_count, sizeof( UINT8 ), 1, fp);

		p_ptr[i] = (int*)malloc(sizeof(int)*( index_count + 1 ));
		p_ptr[i][0] = index_count;
		for ( int j = 1; j <= index_count; j++ )
		{
			UINT16 val;
			fread(&val, sizeof( UINT16 ), 1, fp);
			p_ptr[i][j] = val;
		}
	}
	fclose(fp);

	m_mesh = load_mesh(v_ptr, vertex_count, p_ptr, poly_count);

	m_offset_x = -100;
	m_offset_z = -100;
	m_scale = 0.02;

	CString wp_file;
	wp_file.Format(_T("./wp/%s.wp"), AfxGetApp()->m_lpCmdLine);

	fp = _wfopen(wp_file.GetBuffer(0), _T("rb"));

	m_wp_list.clear();

	uint32_t wp_size;
	fread((void*)&wp_size, sizeof( uint32_t ), 1, fp);

	for ( uint32_t i = 0; i < wp_size; i++ ) {
		uint32_t x;
		fread((void*)&x, sizeof( uint32_t ), 1, fp);

		uint32_t y;
		fread((void*)&y, sizeof( uint32_t ), 1, fp);

		WayPoint* wp = new WayPoint();
		wp->id = i;
		wp->pt.x = x;
		wp->pt.y = y;
		wp->r = 250;
		wp->check = false;
		m_wp_list[wp->id] = wp;
	}

	for ( uint32_t i = 0; i < wp_size; i++ ) {
		WayPoint* wp = m_wp_list[i];
		uint32_t link_size;
		fread((void*)&link_size, sizeof( uint32_t ), 1, fp);
		for ( uint32_t j = 0; j < link_size; j++ )
		{
			uint32_t link_id;
			fread((void*)&link_id, sizeof( uint32_t ), 1, fp);
			wp->link.insert(link_id);
		}
	}

	USES_CONVERSION;
	m_finder = finder_create(T2A(wp_file.GetBuffer(0)));
	m_start = m_over = NULL;

	AllocConsole();
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	int hCrt = _open_osfhandle((long)handle, _O_TEXT);
	FILE * hf = _fdopen(hCrt, "w");
	*stdout = *hf;

	m_mouse_state = true;
	m_mouse_point = NULL;

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void Cwp_finderDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void Cwp_finderDlg::OnPaint()
{
	if (IsIconic())
	{
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
	}
	else
	{
		CDialogEx::OnPaint();
	}
	UpdateMesh();
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR Cwp_finderDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}

void Cwp_finderDlg::UpdateMesh()
{
	CPen pen(PS_SOLID, 1, RGB(0, 0, 0));
	CClientDC dc(this);
	CPen *open = dc.SelectObject(&pen);

	CBrush brush(RGB(255, 0, 0));
	CBrush *obrush = dc.SelectObject(&brush);

	CBrush brushDoor(RGB(88, 88, 0));
	for ( int i = 0; i < m_mesh->node_size; i++ )
	{
		struct nav_node* node = get_node(m_mesh, i);
		CPoint* pt = new CPoint[node->size];

		if ( node->mask == 0 )
		{
			dc.SelectObject(&brush);
		}
		else
		{
			dc.SelectObject(&brushDoor);
		}
		for ( int j = 0; j < node->size; j++ )
		{
			struct vector3* pos = &m_mesh->vertices[node->poly[j]];

			pt[j].x = pos->x*m_scale + m_offset_x;
			pt[j].y = pos->z*m_scale + m_offset_z;
		}
		dc.Polygon(pt, node->size);
		delete[] pt;
		dc.SelectObject(obrush);
	}

	dc.SelectObject(open);
	dc.SelectObject(obrush);

	std::map<int, WayPoint*>::iterator it = m_wp_list.begin();
	for ( ; it != m_wp_list.end(); it++ ) {
		WayPoint* wp = it->second;
		CPoint* pt = &wp->pt;
		CBrush brush_check_false(RGB(0, 0, 255));
		CBrush brush_check_true(RGB(0, 255, 0));
		if ( wp->check ) {
			dc.SelectObject(&brush_check_true);
		}
		else {
			dc.SelectObject(&brush_check_false);
		}

		CPoint mesh_pt_from;
		mesh2editor(&wp->pt, &mesh_pt_from);
		std::set<int>::iterator it_link = wp->link.begin();
		for ( ; it_link != wp->link.end(); it_link++ )
		{
			int link_id = *it_link;
			std::map<int, WayPoint*>::iterator it_next = m_wp_list.find(link_id);
			if ( it_next != m_wp_list.end() )
			{
				WayPoint* link_wp = it_next->second;

				CPoint mesh_pt_to;

				mesh2editor(&link_wp->pt, &mesh_pt_to);
				dc.MoveTo(mesh_pt_from.x, mesh_pt_from.y);
				dc.LineTo(mesh_pt_to.x, mesh_pt_to.y);
			}
		}

		dc.Ellipse(mesh_pt_from.x - wp->r * m_scale, mesh_pt_from.y - wp->r* m_scale, mesh_pt_from.x + wp->r* m_scale, mesh_pt_from.y + wp->r* m_scale);
	}

	if (m_start)
	{
		CPoint pt;
		mesh2editor(m_start,&pt);

		CBrush brush(RGB(255, 255, 255));
		dc.SelectObject(&brush);
		dc.Ellipse(pt.x - 250 * m_scale, pt.y - 250 * m_scale, pt.x + 250 * m_scale, pt.y + 250 * m_scale);
	}

	if (m_over)
	{
		CPoint pt;
		mesh2editor(m_over, &pt);

		CBrush brush(RGB(0, 0, 0));
		dc.SelectObject(&brush);
		dc.Ellipse(pt.x - 250 * m_scale, pt.y - 250 * m_scale, pt.x + 250 * m_scale, pt.y + 250 * m_scale);
	}

	if (m_path.size() > 0)
	{
		CPen pen(PS_SOLID, 1, RGB(0, 255, 0));
		dc.SelectObject(&pen);

		std::vector<CPoint>::reverse_iterator iter = m_path.rbegin();
		if ( iter != m_path.rend() )
		{
			CPoint pt = *iter;
			CPoint editor_pt;
			mesh2editor(&pt, &editor_pt);
			dc.MoveTo(editor_pt);
		}

		for ( ; iter != m_path.rend(); iter++ )
		{
			CPoint pt = *iter;
			CPoint editor_pt;
			mesh2editor(&pt, &editor_pt);
			dc.LineTo(editor_pt);
			dc.MoveTo(editor_pt);
		}
	}

	if ( m_search.size() > 0 )
	{
		CPen pen(PS_SOLID, 1, RGB(0, 255, 0));
		dc.SelectObject(&pen);

		std::vector<CPoint>::iterator iter = m_search.begin();

		for ( ; iter != m_search.end(); iter++ )
		{
			CPoint pt = *iter;
			CPoint editor_pt;
			mesh2editor(&pt, &editor_pt);
			dc.Ellipse(editor_pt.x - 250 * m_scale, editor_pt.y - 250 * m_scale, editor_pt.x + 250 * m_scale, editor_pt.y + 250 * m_scale);
		}
	}
}

void Cwp_finderDlg::editor2mesh(CPoint* from, CPoint* to)
{
	to->x = ( from->x - m_offset_x ) / m_scale;
	to->y = ( from->y - m_offset_z ) / m_scale;
}

void Cwp_finderDlg::mesh2editor(CPoint* from, CPoint* to)
{
	to->x = from->x * m_scale + m_offset_x;
	to->y = from->y * m_scale + m_offset_z;
}

void Cwp_finderDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnMouseMove(nFlags, point);

	if ( m_mouse_state == false ) {
		m_offset_x = point.x - m_mouse_point->x;
		m_offset_z = point.y - m_mouse_point->y;
		Invalidate();
	}
}


BOOL Cwp_finderDlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	if ( zDelta > 0 )
	{
		m_scale += 0.001;
	}
	else {
		m_scale -= 0.001;
	}
	Invalidate();
	return CDialogEx::OnMouseWheel(nFlags, zDelta, pt);
}


void Cwp_finderDlg::OnMButtonUp(UINT nFlags, CPoint point)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	m_mouse_state = true;
	if ( m_mouse_point ) {
		delete m_mouse_point;
		m_mouse_point = NULL;
	}
	CDialogEx::OnMButtonUp(nFlags, point);
}


void Cwp_finderDlg::OnMButtonDown(UINT nFlags, CPoint point)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	m_mouse_state = false;
	m_mouse_point = new CPoint(point);
	CDialogEx::OnMButtonDown(nFlags, point);
}


void Cwp_finderDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnLButtonUp(nFlags, point);
	if ( m_start )
		delete m_start;
	
	m_start = new CPoint(point);

	editor2mesh(&point, m_start);

	Invalidate();
}


void Cwp_finderDlg::OnRButtonUp(UINT nFlags, CPoint point)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnRButtonUp(nFlags, point);
	if ( m_over )
		delete m_over;

	m_over = new CPoint(point);
	editor2mesh(&point, m_over);

	Invalidate();
}

void OnPathDump(void* ud, int x, int z)
{
	Cwp_finderDlg* self = (Cwp_finderDlg*)ud;
	self->m_path.push_back(CPoint(x, z));
}

void OnSearchDump(void* ud, int x, int z)
{
	Cwp_finderDlg* self = (Cwp_finderDlg*)ud;
	self->m_search.push_back(CPoint(x, z));
}

void Cwp_finderDlg::OnBnClickedButton1()
{
	// TODO:  在此添加控件通知处理程序代码
	if ( m_start == NULL )
	{
		CString str;
		str.Format(_T("始点还没设置"));
		MessageBox(str);
	}
	else if ( m_over == NULL)
	{
		CString str;
		str.Format(_T("终点还没设置"));
		MessageBox(str);
	}

	m_search.clear();

	m_path.clear();

	m_path.push_back(CPoint(m_over->x, m_over->y));

	finder_find(m_finder, m_start->x, m_start->y, m_over->x, m_over->y, OnPathDump, this, OnSearchDump, this);

	m_path.push_back(CPoint(m_start->x, m_start->y));

	Invalidate();
}
