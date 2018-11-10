
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

class CAboutDlg : public CDialogEx
{
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

CAboutDlg::CAboutDlg() : CDialogEx(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// pathfinderDlg 对话框



pathfinderDlg::pathfinderDlg(CWnd* pParent /*=NULL*/)
: CDialogEx(pathfinderDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);

}

void pathfinderDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(pathfinderDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON1, &pathfinderDlg::OnPath)
	ON_WM_MOUSEACTIVATE()
	ON_UPDATE_COMMAND_UI(IDD_PATHFINDERTEST_DIALOG, &pathfinderDlg::OnUpdateIddPathfindertestDialog)
	ON_BN_CLICKED(IDC_BUTTON2, &pathfinderDlg::Straightline)
	ON_WM_LBUTTONUP()
	ON_BN_CLICKED(IDC_BUTTON3, &pathfinderDlg::OnIgnoreLine)
	ON_EN_CHANGE(IDC_EDIT2, &pathfinderDlg::OnEnChangeEdit2)
	ON_EN_CHANGE(IDC_EDIT3, &pathfinderDlg::OnEnChangeEdit3)
	ON_EN_CHANGE(IDC_EDIT4, &pathfinderDlg::OnEnChangeEdit4)
	ON_BN_CLICKED(IDC_BUTTON4, &pathfinderDlg::OnIgnorePath)
	ON_WM_RBUTTONUP()
	ON_WM_CLOSE()
	ON_BN_CLICKED(IDC_CHECK1, &pathfinderDlg::OnCheck)
	ON_EN_CHANGE(IDC_EDIT5, &pathfinderDlg::OnEnChangeEdit5)
	ON_EN_CHANGE(IDC_EDIT6, &pathfinderDlg::OnEnChangeEdit6)
	ON_WM_MOUSEWHEEL()
	ON_WM_MBUTTONUP()
	ON_WM_MBUTTONDOWN()
	ON_WM_MOUSEMOVE()
END_MESSAGE_MAP()


// pathfinderDlg 消息处理程序

BOOL pathfinderDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();
	//_CrtSetBreakAlloc(3850);
	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT(( IDM_ABOUTBOX & 0xFFF0 ) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if ( pSysMenu != NULL )
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if ( !strAboutMenu.IsEmpty() )
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
		for ( int j = 0; j < 3;j++ )
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
		for ( int j = 1; j <= index_count;j++ )
		{
			UINT16 val;
			fread(&val, sizeof( UINT16 ), 1, fp);
			p_ptr[i][j] = val;
		}
	}
	fclose(fp);

	int now = time(NULL);
//#define EDITOR_TILE
	this->m_mesh = load_mesh(v_ptr, vertex_count, p_ptr, poly_count);
#ifdef EDITOR_TILE
	this->m_mesh->tile = create_tile(this->m_mesh, 400);
#else
	CString tile_file;
	tile_file.Format(_T("./nav/%s.tile"), AfxGetApp()->m_lpCmdLine);
	fp = _wfopen(tile_file.GetBuffer(0), _T("rb"));

	uint32_t tile_unit;
	fread(&tile_unit, sizeof( uint32_t ), 1, fp);

	uint32_t count = 0;
	fread(&count, sizeof( uint32_t ), 1, fp);

	this->m_mesh->tile = ( struct nav_tile* )malloc(sizeof( struct nav_tile )*count);
	memset(this->m_mesh->tile, 0, sizeof( struct nav_tile )*count);

	uint32_t i;
	for ( i = 0; i < count; i++ ) {

		struct nav_tile* tile = &this->m_mesh->tile[i];

		uint32_t size;
		fread(&size, sizeof( uint32_t ), 1, fp);

		tile->offset = tile->size = size;
		tile->node = NULL;
		if ( tile->offset != 0 ) {
			tile->node = (int*)malloc(sizeof(int)*tile->offset);
			uint32_t j;
			for ( j = 0; j < size; j++ ) {
				uint32_t val;
				fread(&val, sizeof( uint32_t ), 1, fp);
				tile->node[j] = val;
			}
		}

		float x, z;
		fread(&x, sizeof( float ), 1, fp);
		fread(&z, sizeof( float ), 1, fp);

		tile->center.x = x;
		tile->center.y = 0;
		tile->center.z = z;

		int center_node;
		fread(&center_node, sizeof( int ), 1, fp);
		tile->center_node = center_node;
	}
	fclose(fp);

	this->m_mesh->tile_unit = tile_unit;
	this->m_mesh->tile_width = this->m_mesh->width / tile_unit + 1;
	this->m_mesh->tile_heigh = this->m_mesh->heigh / tile_unit + 1;

#endif

	m_offset_x = 100;
	m_offset_z = -100;
	m_scale = 0.02f;

	m_mouse_state = true;
	m_mouse_point = NULL;

	m_poly_begin = -1;
	m_poly_over = -1;
	
	m_pt_over = m_pt_begin = NULL;

	for ( int i = 0; i < vertex_count; i++ )
	{
		free(v_ptr[i]);
	}
	free(v_ptr);
	for ( int i = 0; i < poly_count; i++ )
	{
		free(p_ptr[i]);
	}
	free(p_ptr);

	CString str;
	str.Format(_T("%d"), m_offset_x);
	( (CEdit*)GetDlgItem(IDC_EDIT2) )->SetWindowTextW(str);
	str.Format(_T("%d"), m_offset_z);
	( (CEdit*)GetDlgItem(IDC_EDIT3) )->SetWindowTextW(str);
	str.Format(_T("%f"), m_scale);
	( (CEdit*)GetDlgItem(IDC_EDIT4) )->SetWindowTextW(str);

	AllocConsole();
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	int hCrt = _open_osfhandle((long)handle, _O_TEXT);
	FILE * hf = _fdopen(hCrt, "w");
	*stdout = *hf;

	printf("load tile time:%ld\n", time(NULL) - now);

	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void pathfinderDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ( ( nID & 0xFFF0 ) == IDM_ABOUTBOX )
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

void pathfinderDlg::OnPaint()
{
	if ( IsIconic() )
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>( dc.GetSafeHdc() ), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = ( rect.Width() - cxIcon + 1 ) / 2;
		int y = ( rect.Height() - cyIcon + 1 ) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);


	}
	else
	{
		CDialogEx::OnPaint();
	}

	DrawMap();
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR pathfinderDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>( m_hIcon );
}

void pathfinderDlg::DrawPath(struct vector3* path, int size)
{
	CClientDC dc(this);
	CPen pen(PS_SOLID, 1, RGB(255, 255, 255));
	CPen* open = dc.SelectObject(&pen);


	dc.MoveTo(path[0].x*m_scale + m_offset_x, path[0].z*m_scale + m_offset_z);
	for ( int i = 1; i < size; i++ )
	{
		dc.LineTo(path[i].x*m_scale + m_offset_x, path[i].z*m_scale + m_offset_z);
		dc.MoveTo(path[i].x*m_scale + m_offset_x, path[i].z*m_scale + m_offset_z);
	}

	dc.SelectObject(open);
}


int lua_draw(lua_State* L)
{
	pathfinderDlg* self = (pathfinderDlg*)lua_touserdata(L, 1);
	struct nav_path* path = (nav_path*)lua_touserdata(L, 2);

	self->DrawPath(path->wp, path->offset);
	return 0;
}


void OnSearchDump(void* self, int index)
{
	pathfinderDlg* dlgPtr = (pathfinderDlg*)self;
	CClientDC dc(dlgPtr);
	CBrush brush(RGB(123, 255, 0));
	dc.SelectObject(&brush);

	struct nav_node* node = get_node(dlgPtr->m_mesh, index);
	CPoint* pt = new CPoint[node->size];
	for ( int j = 0; j < node->size; j++ )
	{
		struct vector3* pos = &dlgPtr->m_mesh->vertices[node->poly[j]];
		pt[j].x = pos->x*dlgPtr->m_scale + dlgPtr->m_offset_x;
		pt[j].y = pos->z*dlgPtr->m_scale + dlgPtr->m_offset_z;
	}
	dc.Polygon(pt, node->size);
	delete[] pt;
	Sleep(5);
}

void pathfinderDlg::OnPath()
{
	// TODO:  在此添加控件通知处理程序代码

	if ( m_poly_begin == -1 )
	{
		CString str;
		str.Format(_T("始点还没设置"));
		MessageBox(str);
	}
	else if ( m_poly_over == -1 )
	{
		CString str;
		str.Format(_T("终点还没设置"));
		MessageBox(str);
	}
	else
	{
		for ( int i = 0; i < 8; i++ )
		{
			set_mask(&m_mesh->mask_ctx, i, 1);
		}
		//set_mask(&m_mesh->mask_ctx,1,0);

		struct vector3 ptBegin;
		ptBegin.x = (double)( m_pt_begin->x - m_offset_x ) / m_scale;
		ptBegin.z = (double)( m_pt_begin->z - m_offset_z ) / m_scale;
		struct vector3 ptOver;
		ptOver.x = (double)( m_pt_over->x - m_offset_x ) / m_scale;
		ptOver.z = (double)( m_pt_over->z - m_offset_z ) / m_scale;

		vector3 vt;
		struct nav_path* path = astar_find(m_mesh, &ptBegin, &ptOver, NULL, NULL);

		if (path)
		{
			DrawPath(path->wp, path->offset);
		}
		

	}
}


int pathfinderDlg::OnMouseActivate(CWnd* pDesktopWnd, UINT nHitTest, UINT message)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值

	return CDialogEx::OnMouseActivate(pDesktopWnd, nHitTest, message);
}


void pathfinderDlg::DrawMap()
{
	CPen pen(PS_SOLID, 1, RGB(0, 0, 0));
	CClientDC dc(this);
	CPen *pOldPen = dc.SelectObject(&pen);

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

	obrush = dc.SelectObject(&brush);
	CBrush brush_empty0(RGB(255, 255, 0));
	CBrush brush_empty1(RGB(255, 0, 0));
	CBrush brush_empty2(RGB(88, 88, 0));

	int check = ( (CButton *)GetDlgItem(IDC_CHECK1) )->GetCheck();
	if ( check )
	{
		for ( int i = 0; i < m_mesh->tile_width * m_mesh->tile_heigh; i++ )
		{
			struct nav_tile* tile = &m_mesh->tile[i];
			CPoint pt[4];

			if ( tile->offset == 0 )
				dc.SelectObject(&brush_empty0);
			else
				dc.SelectObject(&brush_empty1);


			for ( int j = 0; j < 4; j++ )
			{
				struct vector3* pos = &tile->pos[j];

				pt[j].x = pos->x*m_scale + m_offset_x;
				pt[j].y = pos->z*m_scale + m_offset_z;
			}
			dc.Polygon(pt, 4);
		}
	}

	dc.SelectObject(obrush);

	if ( m_poly_begin != -1 )
	{
		CBrush brush(RGB(0, 255, 0));
		dc.SelectObject(&brush);

		struct nav_node* node = get_node(m_mesh, m_poly_begin);
		CPoint* pt = new CPoint[node->size];
		for ( int j = 0; j < node->size; j++ )
		{
			struct vector3* pos = &m_mesh->vertices[node->poly[j]];
			pt[j].x = pos->x*m_scale + m_offset_x;
			pt[j].y = pos->z*m_scale + m_offset_z;
		}
		dc.Polygon(pt, node->size);
		delete[] pt;
	}

	if ( m_poly_over != -1 )
	{
		CBrush brush(RGB(0, 0, 255));
		dc.SelectObject(&brush);

		struct nav_node* node = get_node(m_mesh, m_poly_over);
		CPoint* pt = new CPoint[node->size];
		for ( int j = 0; j < node->size; j++ )
		{
			struct vector3* pos = &m_mesh->vertices[node->poly[j]];
			pt[j].x = pos->x*m_scale + m_offset_x;
			pt[j].y = pos->z*m_scale + m_offset_z;
		}
		dc.Polygon(pt, node->size);
		delete[] pt;
	}


	if ( m_pt_begin != NULL )
	{
		CBrush brush(RGB(50, 50, 50));
		dc.SelectObject(&brush);
		dc.Ellipse(m_pt_begin->x - 3, m_pt_begin->z - 3, m_pt_begin->x + 3, m_pt_begin->z + 3);
		int x = ( ( m_pt_begin->x - m_offset_x ) / m_scale - m_mesh->lt.x ) / m_mesh->tile_unit;
		int z = ( ( m_pt_begin->z - m_offset_z ) / m_scale - m_mesh->lt.z ) / m_mesh->tile_unit;
		int index = x + z * m_mesh->tile_width;
		struct nav_tile* tile = &m_mesh->tile[index];

		//格子跨跃多少个多边形
		/*	for (int i = 0;i < tile->offset;i++)
			{
			int node_id = tile->node[i];

			CBrush brush(RGB(111,111,66));
			dc.SelectObject(&brush);

			struct nav_node* node = get_node(m_mesh,node_id);
			CPoint* pt0 = new CPoint[node->size];
			for (int j = 0; j < node->size;j++)
			{
			struct vector3* pos = &m_mesh->vertices[node->poly[j]];
			pt0[j].x = pos->x*m_scale+m_offset_x;
			pt0[j].y = pos->z*m_scale+m_offset_z;
			}
			dc.Polygon(pt0,node->size);
			delete[] pt0;
			}
			*/

		//CPoint pt[4];
		//CBrush brush00(RGB(99, 99, 99));
		//dc.SelectObject(&brush00);

		//for ( int j = 0; j < 4; j++ )
		//{
		//	struct vector3* pos = &tile->pos[j];

		//	pt[j].x = pos->x*m_scale + m_offset_x;
		//	pt[j].y = pos->z*m_scale + m_offset_z;
		//}
		//dc.Polygon(pt, 4);

		//for (int j = 0; j < 4;j++)
		//{
		//	struct vector3* pos = &tile->pos[j];

		//	pt[j].x = pos->x*m_scale+m_offset_x +300;
		//	pt[j].y = pos->z*m_scale+m_offset_z;
		//}
		//dc.Polygon(pt,4);
	}

	if ( m_pt_over != NULL )
	{
		CBrush brush(RGB(250, 50, 50));
		dc.SelectObject(&brush);
		dc.Ellipse(m_pt_over->x - 3, m_pt_over->z - 3, m_pt_over->x + 3, m_pt_over->z + 3);
	}

	dc.SelectObject(pOldPen);
	dc.SelectObject(obrush);

}

void pathfinderDlg::DrawBegin(CPoint& pos)
{
	double nav_x = (double)( pos.x - m_offset_x ) / m_scale;
	double nav_z = (double)( pos.y - m_offset_z ) / m_scale;

	printf("nav begin:%f,%f\n", nav_x, nav_z);

	struct nav_node* node = search_node(m_mesh, nav_x, 0, nav_z);
	if ( node == NULL )
		return;

	if ( m_pt_begin != NULL ) {
		free(m_pt_begin);
		m_pt_begin = NULL;
	}

	m_poly_begin = node->id;

	m_pt_begin = (vector3*)malloc(sizeof( *m_pt_begin ));
	m_pt_begin->x = pos.x;
	m_pt_begin->z = pos.y;

	
	printf("nav node:%d\n", node->id);
	for ( int i = 0; i < node->size;i++ )
		printf("%d\t", node->poly[i]);
	printf("\n");

	double height;
	if ( point_height(m_mesh, nav_x, nav_z, &height) ) {
		printf("heigh:%f\n", height);
	}
	Invalidate();
}

void OnAroundDump(void* self, int index)
{
	pathfinderDlg* dlgPtr = (pathfinderDlg*)self;
	CClientDC dc(dlgPtr);
	CBrush brush(RGB(66, 66, 66));

	struct nav_tile* tile = &dlgPtr->m_mesh->tile[index];
	CPoint pt[4];


	CBrush* obrush = dc.SelectObject(&brush);

	for ( int j = 0; j < 4; j++ )
	{
		struct vector3* pos = &tile->pos[j];

		pt[j].x = pos->x*dlgPtr->m_scale + dlgPtr->m_offset_x;
		pt[j].y = pos->z*dlgPtr->m_scale + dlgPtr->m_offset_z;
	}
	dc.Polygon(pt, 4);
	dc.SelectObject(obrush);
}

void pathfinderDlg::DrawOver(CPoint& pos)
{
	double nav_x = (double)( pos.x - m_offset_x ) / m_scale;
	double nav_z = (double)( pos.y - m_offset_z ) / m_scale;

	struct nav_node* node = NULL;
	int node_index;
	if ( !point_movable(m_mesh, nav_x, nav_z, 10, NULL) )
	{
		vector3* vt = around_movable(m_mesh, nav_x, nav_z, 5, &node_index, OnAroundDump, this);
		if ( !vt ) {
			printf("%f,%f\n", nav_x, nav_z);
			return;
		}
			
			
		node = get_node(m_mesh, node_index);
		
		CBrush brush(RGB(66, 88, 188));
		CClientDC dc(this);
		CBrush* obrush = dc.SelectObject(&brush);
		dc.Ellipse(vt->x*m_scale + m_offset_x - 3, vt->z*m_scale + m_offset_z - 3, vt->x*m_scale + m_offset_x + 3, vt->z*m_scale + m_offset_z + 3);
		dc.SelectObject(obrush);
		pos.x = vt->x*m_scale + m_offset_x;
		pos.y = vt->z*m_scale + m_offset_z;

		if ( m_pt_over != NULL )
		{
			free(m_pt_over);
			m_pt_over = NULL;
		}

		m_pt_over = (vector3*)malloc(sizeof( *m_pt_over ));
		m_pt_over->x = vt->x*m_scale + m_offset_x;
		m_pt_over->z = vt->z*m_scale + m_offset_z;

		printf("nav over:%f,%f\n", vt->x, vt->z);
		printf("nav node:%d\n", node->id);

		vector3 start;
		start.x = nav_x;
		start.z = nav_z;
		vector3 result;
		bool ok = raycast(m_mesh, vt, &start, &result, NULL, this);
		if ( ok ){
			dc.Ellipse(result.x*m_scale + m_offset_x - 3, result.z*m_scale + m_offset_z - 3, result.x*m_scale + m_offset_x + 3, result.z*m_scale + m_offset_z + 3);
			dc.SelectObject(obrush);
		}
	}
	else
	{
		node = search_node(m_mesh, nav_x, 0, nav_z);

		if ( m_pt_over != NULL )
		{
			free(m_pt_over);
			m_pt_over = NULL;
		}

		m_pt_over = (vector3*)malloc(sizeof( *m_pt_over ));
		m_pt_over->x = pos.x;
		m_pt_over->z = pos.y;

		printf("nav over:%f,%f\n", nav_x,nav_z);
		printf("nav node:%d\n", node->id);

		Invalidate();
	}

	for ( int i = 0; i < node->size; i++ )
		printf("%d\t", node->poly[i]);
	printf("\n");

	m_poly_over = node->id;

	
}

void pathfinderDlg::OnUpdateIddPathfindertestDialog(CCmdUI *pCmdUI)
{
	// TODO:  在此添加命令更新用户界面处理程序代码
	DrawMap();
}



void pathfinderDlg::Straightline()
{
	// TODO:  在此添加控件通知处理程序代码
	if ( m_poly_begin == -1 )
	{
		CString str;
		str.Format(_T("始点还没设置"));
		MessageBox(str);
	}
	else if ( m_poly_over == -1 )
	{
		CString str;
		str.Format(_T("终点还没设置"));
		MessageBox(str);
	}
	else
	{
		for ( int i = 0; i < 8; i++ )
		{
			set_mask(&m_mesh->mask_ctx, i, 1);
		}
		set_mask(&m_mesh->mask_ctx, 1, 0);

		vector3 vt0;
		vt0.x = (double)( m_pt_begin->x - m_offset_x ) / m_scale;
		vt0.y = 0;
		vt0.z = (double)( m_pt_begin->z - m_offset_z ) / m_scale;

		vector3 vt1;
		vt1.x = (double)( m_pt_over->x - m_offset_x ) / m_scale;
		vt1.y = 0;
		vt1.z = (double)( m_pt_over->z - m_offset_z ) / m_scale;

		POINT from;
		from.x = vt0.x*m_scale + m_offset_x;
		from.y = vt0.z*m_scale + m_offset_z;

		CPen pen(PS_SOLID, 1, RGB(255, 255, 255));
		CClientDC dc(this);
		CPen *open = dc.SelectObject(&pen);
		dc.MoveTo(from);

		vector3 vt;
		bool ok = raycast(m_mesh, &vt0, &vt1, &vt, NULL, this);

		POINT to;
		to.x = vt.x*m_scale + m_offset_x;
		to.y = vt.z*m_scale + m_offset_z;

		dc.LineTo(to);
		dc.SelectObject(open);
	}
}


void pathfinderDlg::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnLButtonDown(nFlags, point);
}


void pathfinderDlg::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值


	this->DrawBegin(point);

	CDialogEx::OnLButtonUp(nFlags, point);
}



void pathfinderDlg::OnIgnoreLine()
{
	// TODO:  在此添加控件通知处理程序代码
	if ( m_poly_begin == NULL )
	{
		CString str;
		str.Format(_T("始点还没设置"));
		MessageBox(str);
	}
	else if ( m_poly_over == NULL )
	{
		CString str;
		str.Format(_T("终点还没设置"));
		MessageBox(str);
	}
	else
	{
		for ( int i = 0; i < 8; i++ )
		{
			set_mask(&m_mesh->mask_ctx, i, 1);
		}
		vector3 vt0;
		vt0.x = (double)( m_pt_begin->x - m_offset_x ) / m_scale;
		vt0.y = 0;
		vt0.z = (double)( m_pt_begin->z - m_offset_z ) / m_scale;

		vector3 vt1;
		vt1.x = (double)( m_pt_over->x - m_offset_x ) / m_scale;
		vt1.y = 0;
		vt1.z = (double)( m_pt_over->z - m_offset_z ) / m_scale;


		vector3 vt;
		bool ok = raycast(m_mesh, &vt0, &vt1, &vt, OnSearchDump, this);

		POINT from;
		from.x = vt0.x*m_scale + m_offset_x;
		from.y = vt0.z*m_scale + m_offset_z;

		POINT to;
		to.x = vt.x*m_scale + m_offset_x;
		to.y = vt.z*m_scale + m_offset_z;

		CPen pen(PS_SOLID, 1, RGB(255, 255, 255));
		CClientDC dc(this);
		CPen *open = dc.SelectObject(&pen);
		dc.MoveTo(from);
		dc.LineTo(to);
		dc.SelectObject(open);

		set_mask(&m_mesh->mask_ctx, 0, 1);
	}
}


void pathfinderDlg::OnEnChangeEdit2()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。



	// TODO:  在此添加控件通知处理程序代码
	CString str;
	( (CEdit*)GetDlgItem(IDC_EDIT2) )->GetWindowTextW(str);
	m_offset_x = _ttoi(str);
	Invalidate();
}


void pathfinderDlg::OnEnChangeEdit3()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。



	// TODO:  在此添加控件通知处理程序代码
	CString str;
	( (CEdit*)GetDlgItem(IDC_EDIT3) )->GetWindowTextW(str);
	m_offset_z = _ttoi(str);
	Invalidate();
}


void pathfinderDlg::OnEnChangeEdit4()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	CString str;
	( (CEdit*)GetDlgItem(IDC_EDIT4) )->GetWindowTextW(str);
	m_scale = _ttof(str);
	Invalidate();

	// TODO:  在此添加控件通知处理程序代码
}


void pathfinderDlg::OnIgnorePath()
{
	if ( m_poly_begin == -1 )
	{
		CString str;
		str.Format(_T("始点还没设置"));
		MessageBox(str);
	}
	else if ( m_poly_over == -1 )
	{
		CString str;
		str.Format(_T("终点还没设置"));
		MessageBox(str);
	}
	else
	{
		for ( int i = 0; i < 8; i++ )
			set_mask(&m_mesh->mask_ctx, i, 1);

		struct vector3 ptBegin;
		ptBegin.x = (double)( m_pt_begin->x - m_offset_x ) / m_scale;
		ptBegin.z = (double)( m_pt_begin->z - m_offset_z ) / m_scale;
		struct vector3 ptOver;
		ptOver.x = (double)( m_pt_over->x - m_offset_x ) / m_scale;
		ptOver.z = (double)( m_pt_over->z - m_offset_z ) / m_scale;

		vector3 vt;
		struct nav_path* path = astar_find(m_mesh, &ptBegin, &ptOver, OnSearchDump, this);

		if ( path )
		{
			DrawPath(path->wp, path->offset);
		}
	}
}


void pathfinderDlg::OnRButtonUp(UINT nFlags, CPoint point)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	this->DrawOver(point);
	CDialogEx::OnRButtonUp(nFlags, point);
}


void pathfinderDlg::OnClose()
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	release_mesh(m_mesh);
	_CrtDumpMemoryLeaks();
	CDialogEx::OnClose();
}

void pathfinderDlg::OnCheck()
{
	// TODO:  在此添加控件通知处理程序代码
	Invalidate();
}

void pathfinderDlg::OnEnChangeEdit5()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	
}


void pathfinderDlg::OnEnChangeEdit6()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码

}


BOOL pathfinderDlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值
	if ( zDelta > 0 )
	{
		m_scale += 0.001;
	}
	else {
		m_scale -= 0.001;
	}

	CString str;
	str.Format(_T("%f"), m_scale);

	( (CEdit*)GetDlgItem(IDC_EDIT4) )->SetWindowTextW(str);

	Invalidate();
	return CDialogEx::OnMouseWheel(nFlags, zDelta, pt);
}


void pathfinderDlg::OnMButtonUp(UINT nFlags, CPoint point)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnMButtonUp(nFlags, point);
	m_mouse_state = true;
	if ( m_mouse_point ) {
		delete m_mouse_point;
		m_mouse_point = NULL;
	}
}


void pathfinderDlg::OnMButtonDown(UINT nFlags, CPoint point)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnMButtonDown(nFlags, point);

	m_mouse_state = false;
	m_mouse_point = new CPoint(point);
}


void pathfinderDlg::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO:  在此添加消息处理程序代码和/或调用默认值

	CDialogEx::OnMouseMove(nFlags, point);
	if ( m_mouse_state == false ) {
		m_offset_x = point.x - m_mouse_point->x;
		m_offset_z = point.y - m_mouse_point->y;

		CString str;
		str.Format(_T("%d"), m_offset_x);

		( (CEdit*)GetDlgItem(IDC_EDIT2) )->SetWindowTextW(str);

		str.Format(_T("%d"), m_offset_z);

		( (CEdit*)GetDlgItem(IDC_EDIT3) )->SetWindowTextW(str);

		Invalidate();
	}
}
