
// wp_finderDlg.cpp : ʵ���ļ�
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


// ����Ӧ�ó��򡰹��ڡ��˵���� CAboutDlg �Ի���

class CAboutDlg : public CDialogEx {
public:
	CAboutDlg();

	// �Ի�������
	enum { IDD = IDD_ABOUTBOX };

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV ֧��

	// ʵ��
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


// CWpFinderDlg �Ի���



CWpFinderDlg::CWpFinderDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CWpFinderDlg::IDD, pParent) {
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CWpFinderDlg::DoDataExchange(CDataExchange* pDX) {
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CWpFinderDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_WM_MOUSEMOVE()
	ON_WM_MOUSEWHEEL()
	ON_WM_MBUTTONUP()
	ON_WM_MBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONUP()
	ON_BN_CLICKED(IDC_BUTTON1, &CWpFinderDlg::OnBnClickedButton1)
END_MESSAGE_MAP()


// CWpFinderDlg ��Ϣ�������

BOOL CWpFinderDlg::OnInitDialog() {
	CDialogEx::OnInitDialog();

	// ��������...���˵�����ӵ�ϵͳ�˵��С�

	// IDM_ABOUTBOX ������ϵͳ���Χ�ڡ�
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

	// ���ô˶Ի����ͼ�ꡣ  ��Ӧ�ó��������ڲ��ǶԻ���ʱ����ܽ��Զ�
	//  ִ�д˲���
	SetIcon(m_hIcon, TRUE);			// ���ô�ͼ��
	SetIcon(m_hIcon, FALSE);		// ����Сͼ��

	// TODO:  �ڴ���Ӷ���ĳ�ʼ������
	AllocConsole();
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	int hCrt = _open_osfhandle((long)handle, _O_TEXT);
	FILE* hf = _fdopen(hCrt, "w");
	*stdout = *hf;

	CString navTile;
	navTile.Format(_T("./nav/%s"), AfxGetApp()->m_lpCmdLine);
	FILE* fp = _wfopen(navTile.GetBuffer(0), _T("rb"));

	USES_CONVERSION;
	char* pFileName = T2A(navTile);

	m_mesh_finder = NavPathFinder::LoadMeshEx(pFileName);

	CString wpFile;
	wpFile.Format(_T("./wp/%s.wp"), AfxGetApp()->m_lpCmdLine);
	pFileName = T2A(wpFile);
	m_finder = new WpPathFinder((const char*)pFileName);

	m_offset_x = -100;
	m_offset_z = -100;
	m_scale = 0.02;

	m_wp_list.clear();

	for (unsigned int i = 0; i < m_finder->size_; i++) {
		WpPathFinder::WpNode* node = &m_finder->node_[i];
		WayPoint* wp = new WayPoint();
		wp->id = i;
		wp->pt.x = node->pos_.x();
		wp->pt.y = node->pos_.y();
		wp->r = 250;
		wp->check = false;
		m_wp_list[wp->id] = wp;
		for (unsigned int j = 0; j < node->size_; j++) {
			wp->link.insert(node->link_[j]);
		}
	}

	m_start = m_over = NULL;

	m_mouse_state = true;
	m_mouse_point = NULL;

	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

void CWpFinderDlg::OnSysCommand(UINT nID, LPARAM lParam) {
	if ((nID & 0xFFF0) == IDM_ABOUTBOX) {
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	} else {
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// �����Ի��������С����ť������Ҫ����Ĵ���
//  �����Ƹ�ͼ�ꡣ  ����ʹ���ĵ�/��ͼģ�͵� MFC Ӧ�ó���
//  �⽫�ɿ���Զ���ɡ�

void CWpFinderDlg::OnPaint() {
	if (IsIconic()) {
		CPaintDC dc(this); // ���ڻ��Ƶ��豸������

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// ʹͼ���ڹ����������о���
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// ����ͼ��
		dc.DrawIcon(x, y, m_hIcon);
	} else {
		CDialogEx::OnPaint();
	}
	UpdateMesh();
}

//���û��϶���С������ʱϵͳ���ô˺���ȡ�ù��
//��ʾ��
HCURSOR CWpFinderDlg::OnQueryDragIcon() {
	return static_cast<HCURSOR>(m_hIcon);
}

void CWpFinderDlg::UpdateMesh() {
	CPen pen(PS_SOLID, 1, RGB(0, 0, 0));
	CClientDC dc(this);
	CPen* open = dc.SelectObject(&pen);

	CBrush brush(RGB(255, 0, 0));
	CBrush* obrush = dc.SelectObject(&brush);

	CBrush brushDoor(RGB(88, 88, 0));
	for (unsigned int i = 0; i < m_mesh_finder->mesh_->node_.size(); i++) {
		NavNode* node = m_mesh_finder->GetNode(i);
		CPoint* pt = new CPoint[node->size_];

		if (node->mask_ == 0) {
			dc.SelectObject(&brush);
		} else {
			dc.SelectObject(&brushDoor);
		}
		for (int j = 0; j < node->size_; j++) {
			Math::Vector3* pos = &m_mesh_finder->mesh_->vertice_[node->vertice_[j]];

			pt[j].x = pos->x() * m_scale + m_offset_x;
			pt[j].y = pos->z() * m_scale + m_offset_z;
		}
		dc.Polygon(pt, node->size_);
		delete[] pt;
		dc.SelectObject(obrush);
	}

	dc.SelectObject(open);
	dc.SelectObject(obrush);

	std::map<int, WayPoint*>::iterator it = m_wp_list.begin();
	for (; it != m_wp_list.end(); it++) {
		WayPoint* wp = it->second;
		CPoint* pt = &wp->pt;
		CBrush brush_check_false(RGB(0, 0, 255));
		CBrush brush_check_true(RGB(0, 255, 0));
		if (wp->check) {
			dc.SelectObject(&brush_check_true);
		} else {
			dc.SelectObject(&brush_check_false);
		}

		CPoint mesh_pt_from;
		mesh2editor(&wp->pt, &mesh_pt_from);
		std::set<int>::iterator it_link = wp->link.begin();
		for (; it_link != wp->link.end(); it_link++) {
			int link_id = *it_link;
			std::map<int, WayPoint*>::iterator it_next = m_wp_list.find(link_id);
			if (it_next != m_wp_list.end()) {
				WayPoint* link_wp = it_next->second;

				CPoint mesh_pt_to;

				mesh2editor(&link_wp->pt, &mesh_pt_to);
				dc.MoveTo(mesh_pt_from.x, mesh_pt_from.y);
				dc.LineTo(mesh_pt_to.x, mesh_pt_to.y);
			}
		}

		dc.Ellipse(mesh_pt_from.x - wp->r * m_scale, mesh_pt_from.y - wp->r * m_scale, mesh_pt_from.x + wp->r * m_scale, mesh_pt_from.y + wp->r * m_scale);
	}

	if (m_start) {
		CPoint pt;
		mesh2editor(m_start, &pt);

		CBrush brush(RGB(255, 255, 255));
		dc.SelectObject(&brush);
		dc.Ellipse(pt.x - 250 * m_scale, pt.y - 250 * m_scale, pt.x + 250 * m_scale, pt.y + 250 * m_scale);
	}

	if (m_over) {
		CPoint pt;
		mesh2editor(m_over, &pt);

		CBrush brush(RGB(0, 0, 0));
		dc.SelectObject(&brush);
		dc.Ellipse(pt.x - 250 * m_scale, pt.y - 250 * m_scale, pt.x + 250 * m_scale, pt.y + 250 * m_scale);
	}

	if (m_search.size() > 0) {
		CPen pen(PS_SOLID, 1, RGB(0, 255, 0));
		dc.SelectObject(&pen);

		std::vector<CPoint>::iterator iter = m_search.begin();

		for (; iter != m_search.end(); iter++) {
			CPoint pt = *iter;
			CPoint editor_pt;
			mesh2editor(&pt, &editor_pt);
			dc.Ellipse(editor_pt.x - 250 * m_scale, editor_pt.y - 250 * m_scale, editor_pt.x + 250 * m_scale, editor_pt.y + 250 * m_scale);
		}
	}

	if (m_path.size() > 0) {
		CPen pen(PS_SOLID, 1, RGB(123, 255, 0));
		dc.SelectObject(&pen);

		std::vector<CPoint>::reverse_iterator iter = m_path.rbegin();
		if (iter != m_path.rend()) {
			CPoint pt = *iter;
			CPoint editor_pt;
			mesh2editor(&pt, &editor_pt);
			dc.MoveTo(editor_pt);
		}

		for (; iter != m_path.rend(); iter++) {
			CPoint pt = *iter;
			CPoint editor_pt;
			mesh2editor(&pt, &editor_pt);
			dc.LineTo(editor_pt);
			dc.MoveTo(editor_pt);
		}
	}
}

void CWpFinderDlg::editor2mesh(CPoint* from, CPoint* to) {
	to->x = (from->x - m_offset_x) / m_scale;
	to->y = (from->y - m_offset_z) / m_scale;
}

void CWpFinderDlg::mesh2editor(CPoint* from, CPoint* to) {
	to->x = from->x * m_scale + m_offset_x;
	to->y = from->y * m_scale + m_offset_z;
}

void CWpFinderDlg::OnMouseMove(UINT nFlags, CPoint point) {
	// TODO:  �ڴ������Ϣ�����������/�����Ĭ��ֵ

	CDialogEx::OnMouseMove(nFlags, point);

	if (m_mouse_state == false) {
		m_offset_x = point.x - m_mouse_point->x;
		m_offset_z = point.y - m_mouse_point->y;
		Invalidate();
	}
}


BOOL CWpFinderDlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) {
	// TODO:  �ڴ������Ϣ�����������/�����Ĭ��ֵ
	if (zDelta > 0) {
		m_scale += 0.001;
	} else {
		m_scale -= 0.001;
	}
	Invalidate();
	return CDialogEx::OnMouseWheel(nFlags, zDelta, pt);
}


void CWpFinderDlg::OnMButtonUp(UINT nFlags, CPoint point) {
	// TODO:  �ڴ������Ϣ�����������/�����Ĭ��ֵ
	m_mouse_state = true;
	if (m_mouse_point) {
		delete m_mouse_point;
		m_mouse_point = NULL;
	}
	CDialogEx::OnMButtonUp(nFlags, point);
}


void CWpFinderDlg::OnMButtonDown(UINT nFlags, CPoint point) {
	// TODO:  �ڴ������Ϣ�����������/�����Ĭ��ֵ
	m_mouse_state = false;
	m_mouse_point = new CPoint(point);
	CDialogEx::OnMButtonDown(nFlags, point);
}


void CWpFinderDlg::OnLButtonUp(UINT nFlags, CPoint point) {
	// TODO:  �ڴ������Ϣ�����������/�����Ĭ��ֵ

	CDialogEx::OnLButtonUp(nFlags, point);
	if (m_start)
		delete m_start;

	m_start = new CPoint(point);

	editor2mesh(&point, m_start);

	Invalidate();
}


void CWpFinderDlg::OnRButtonUp(UINT nFlags, CPoint point) {
	// TODO:  �ڴ������Ϣ�����������/�����Ĭ��ֵ

	CDialogEx::OnRButtonUp(nFlags, point);
	if (m_over)
		delete m_over;

	m_over = new CPoint(point);
	editor2mesh(&point, m_over);

	Invalidate();
}

void OnPathDump(void* ud, int x, int z) {
	CWpFinderDlg* self = (CWpFinderDlg*)ud;
	self->m_path.push_back(CPoint(x, z));
}

void OnSearchDump(void* ud, const Math::Vector2& pos) {
	CWpFinderDlg* self = (CWpFinderDlg*)ud;
	self->m_search.push_back(CPoint(pos.x(), pos.y()));

	CPoint pt(pos.x(), pos.y());
	CPoint editor_pt;
	self->mesh2editor(&pt, &editor_pt);
	CClientDC dc(self);
	CBrush brush(RGB(0, 255, 0));
	CBrush* obrush = dc.SelectObject(&brush);
	dc.Ellipse(editor_pt.x - 250 * self->m_scale, editor_pt.y - 250 * self->m_scale, editor_pt.x + 250 * self->m_scale, editor_pt.y + 250 * self->m_scale);
	dc.SelectObject(obrush);
	Sleep(100);
}

void CWpFinderDlg::OnBnClickedButton1() {
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	if (m_start == NULL) {
		CString str;
		str.Format(_T("ʼ�㻹û����"));
		MessageBox(str);
	} else if (m_over == NULL) {
		CString str;
		str.Format(_T("�յ㻹û����"));
		MessageBox(str);
	}
	m_search.clear();

	m_path.clear();

	m_finder->SetDebugCallback(OnSearchDump, this);

	std::vector<const Math::Vector2*> result;
	Math::Vector2 start((float)m_start->x, (float)m_start->y);
	Math::Vector2 over((float)m_over->x, (float)m_over->y);
	if (m_finder->Find(start, over, result) >= 0) {
		m_path.push_back(CPoint(m_start->x, m_start->y));
		for (unsigned int i = 0; i < result.size(); i++) {
			m_path.push_back(CPoint(result[i]->x(), result[i]->y()));
		}
		m_path.push_back(CPoint(m_over->x, m_over->y));
	}

	Sleep(1000);
	Invalidate();
}
