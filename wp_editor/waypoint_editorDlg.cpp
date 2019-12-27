
// waypoint_editorDlg.cpp : ʵ���ļ�
//

#include "stdafx.h"
#include "waypoint_editor.h"
#include "waypoint_editorDlg.h"
#include "afxdialogex.h"
#include <io.h>    
#include <fcntl.h>  
#include <fstream>
#include "MemoryStream.h"
#include "Util.h"
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


// Cwaypoint_editorDlg �Ի���



Cwaypoint_editorDlg::Cwaypoint_editorDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(Cwaypoint_editorDlg::IDD, pParent) {
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void Cwaypoint_editorDlg::DoDataExchange(CDataExchange* pDX) {
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(Cwaypoint_editorDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_UPDATE_COMMAND_UI(IDD_WAYPOINT_EDITOR_DIALOG, &Cwaypoint_editorDlg::OnUpdateIddWaypointEditorDialog)
	ON_WM_MOUSEMOVE()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	//	ON_WM_MOUSEHWHEEL()
	ON_WM_MOUSEWHEEL()
	ON_WM_RBUTTONUP()
	ON_BN_CLICKED(IDC_BUTTON1, &Cwaypoint_editorDlg::OnBnClickedButton1)
	ON_WM_MOUSEHWHEEL()
	ON_WM_MBUTTONDOWN()
	ON_WM_MBUTTONUP()
	ON_BN_CLICKED(IDC_BUTTON2, &Cwaypoint_editorDlg::OnBnClickedButton2)
END_MESSAGE_MAP()


// Cwaypoint_editorDlg ��Ϣ�������

BOOL Cwaypoint_editorDlg::OnInitDialog() {
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

	CString navTile;
	navTile.Format(_T("./nav/%s"), AfxGetApp()->m_lpCmdLine);
	FILE* fp = _wfopen(navTile.GetBuffer(0), _T("rb"));

	USES_CONVERSION;
	char * pFileName = T2A(navTile);

	m_finder = NavPathFinder::LoadMeshEx(pFileName);
	m_offset_x = -100;
	m_offset_z = -100;
	m_scale = 0.02;

	AllocConsole();
	HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
	int hCrt = _open_osfhandle((long)handle, _O_TEXT);
	FILE * hf = _fdopen(hCrt, "w");
	*stdout = *hf;

	m_mouse_state = true;
	m_mouse_point = NULL;

	m_id_countor = 1;
	m_last_wp = -1;
	return TRUE;  // ���ǽ��������õ��ؼ������򷵻� TRUE
}

void Cwaypoint_editorDlg::OnSysCommand(UINT nID, LPARAM lParam) {
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

void Cwaypoint_editorDlg::OnPaint() {
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
HCURSOR Cwaypoint_editorDlg::OnQueryDragIcon() {
	return static_cast<HCURSOR>(m_hIcon);
}

void Cwaypoint_editorDlg::OnUpdateIddWaypointEditorDialog(CCmdUI *pCmdUI) {
	// TODO:  �ڴ������������û����洦��������
	UpdateMesh();
}

void Cwaypoint_editorDlg::UpdateMesh() {
	CPen pen(PS_SOLID, 1, RGB(0, 0, 0));
	CClientDC dc(this);
	CPen *open = dc.SelectObject(&pen);

	CBrush brush(RGB(255, 0, 0));
	CBrush *obrush = dc.SelectObject(&brush);

	CBrush brushDoor(RGB(88, 88, 0));
	for (int i = 0; i < m_finder->mesh_->node_.size(); i++) {
		NavNode* node = m_finder->GetNode(i);
		CPoint* pt = new CPoint[node->size_];

		if (node->mask_ == 0) {
			dc.SelectObject(&brush);
		} else {
			dc.SelectObject(&brushDoor);
		}
		for (int j = 0; j < node->size_; j++) {
			Math::Vector3* pos = &m_finder->mesh_->vertice_[node->vertice_[j]];

			pt[j].x = pos->x*m_scale + m_offset_x;
			pt[j].y = pos->z*m_scale + m_offset_z;
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
		mesh2editor(wp->pt, mesh_pt_from);
		std::set<int>::iterator it_link = wp->link.begin();
		for (; it_link != wp->link.end(); it_link++) {
			int link_id = *it_link;
			std::map<int, WayPoint*>::iterator it_next = m_wp_list.find(link_id);
			if (it_next != m_wp_list.end()) {
				WayPoint* link_wp = it_next->second;

				CPoint mesh_pt_to;

				mesh2editor(link_wp->pt, mesh_pt_to);
				dc.MoveTo(mesh_pt_from.x, mesh_pt_from.y);
				dc.LineTo(mesh_pt_to.x, mesh_pt_to.y);
			}
		}

		dc.Ellipse(mesh_pt_from.x - wp->r * m_scale, mesh_pt_from.y - wp->r* m_scale, mesh_pt_from.x + wp->r* m_scale, mesh_pt_from.y + wp->r* m_scale);
	}
}

void Cwaypoint_editorDlg::editor2mesh(CPoint& from, CPoint& to) {
	to.x = (from.x - m_offset_x) / m_scale;
	to.y = (from.y - m_offset_z) / m_scale;
}

void Cwaypoint_editorDlg::mesh2editor(CPoint& from, CPoint& to) {
	to.x = from.x * m_scale + m_offset_x;
	to.y = from.y * m_scale + m_offset_z;
}

void Cwaypoint_editorDlg::OnMouseMove(UINT nFlags, CPoint point) {
	// TODO:  �ڴ������Ϣ�����������/�����Ĭ��ֵ

	CDialogEx::OnMouseMove(nFlags, point);
	if (m_mouse_state == false) {
		m_offset_x = point.x - m_mouse_point->x;
		m_offset_z = point.y - m_mouse_point->y;
		Invalidate();
	}
}


void Cwaypoint_editorDlg::OnLButtonDown(UINT nFlags, CPoint point) {
	// TODO:  �ڴ������Ϣ�����������/�����Ĭ��ֵ

	CDialogEx::OnLButtonDown(nFlags, point);
}


void Cwaypoint_editorDlg::OnLButtonUp(UINT nFlags, CPoint point) {
	// TODO:  �ڴ������Ϣ�����������/�����Ĭ��ֵ


	CPoint mesh_pt;
	editor2mesh(point, mesh_pt);
	NavNode* node = m_finder->SearchNode(Math::Vector3((float)mesh_pt.x, 0, (double)mesh_pt.y), 1);
	if (node == NULL)
		return;

	bool hit = false;
	bool del = false;
	std::map<int, WayPoint*>::iterator it = m_wp_list.begin();
	for (; it != m_wp_list.end();) {
		WayPoint* wp = it->second;
		if (wp->Inside(mesh_pt)) {
			if (wp->check) {
				std::set<int>::iterator it_link = wp->link.begin();
				for (; it_link != wp->link.end(); it_link++) {
					WayPoint* link_wp = m_wp_list[*it_link];
					link_wp->link.erase(wp->id);
				}
				delete wp;
				it = m_wp_list.erase(it);
				del = true;
			} else {
				wp->check = true;
				hit = true;
				m_last_wp = wp->id;
				it++;
			}
		} else {
			wp->check = false;
			it++;
		}
	}

	if (!del && !hit) {
		WayPoint* wp = new WayPoint();
		wp->id = m_id_countor++;
		wp->pt.x = mesh_pt.x;
		wp->pt.y = mesh_pt.y;
		wp->r = 250;
		wp->check = true;
		m_wp_list[wp->id] = wp;
		m_last_wp = wp->id;
	}

	Invalidate();

}

BOOL Cwaypoint_editorDlg::OnMouseWheel(UINT nFlags, short zDelta, CPoint pt) {
	// TODO:  �ڴ������Ϣ�����������/�����Ĭ��ֵ
	if (zDelta > 0) {
		m_scale += 0.001;
	} else {
		m_scale -= 0.001;
	}
	Invalidate();
	return CDialogEx::OnMouseWheel(nFlags, zDelta, pt);
}

void Cwaypoint_editorDlg::OnRButtonUp(UINT nFlags, CPoint point) {
	// TODO:  �ڴ������Ϣ�����������/�����Ĭ��ֵ

	CDialogEx::OnRButtonUp(nFlags, point);

	{
		CPoint mesh_pt;
		editor2mesh(point, mesh_pt);
		NavNode* node = m_finder->SearchNode(Math::Vector3((float)mesh_pt.x, 0, (double)mesh_pt.y), 1);
		if (node == NULL)
			return;

		int hit_id = -1;
		std::map<int, WayPoint*>::iterator it = m_wp_list.begin();
		for (; it != m_wp_list.end(); it++) {
			WayPoint* wp = it->second;
			if (wp->Inside(mesh_pt)) {
				hit_id = wp->id;
				break;
			}
		}

		if (hit_id >= 0) {
			if (m_last_wp >= 0) {
				std::map<int, WayPoint*>::iterator it = m_wp_list.find(m_last_wp);
				if (it == m_wp_list.end())
					return;
				WayPoint* last_wp = it->second;
				WayPoint* next_wp = m_wp_list[hit_id];
				std::set<int>::iterator it_set = last_wp->link.find(hit_id);
				if (it_set == last_wp->link.end()) {
					last_wp->link.insert(hit_id);
					next_wp->link.insert(last_wp->id);
				} else {
					last_wp->link.erase(it_set);
					next_wp->link.erase(last_wp->id);
				}
			}
		}

		Invalidate();
	}
}

void Cwaypoint_editorDlg::OnMouseHWheel(UINT nFlags, short zDelta, CPoint pt) {
	// �˹���Ҫ�� Windows Vista ����߰汾��
	// _WIN32_WINNT ���ű��� >= 0x0600��
	// TODO:  �ڴ������Ϣ�����������/�����Ĭ��ֵ

	CDialogEx::OnMouseHWheel(nFlags, zDelta, pt);
}


void Cwaypoint_editorDlg::OnMButtonDown(UINT nFlags, CPoint point) {
	// TODO:  �ڴ������Ϣ�����������/�����Ĭ��ֵ
	CDialogEx::OnMButtonDown(nFlags, point);

	m_mouse_state = false;
	m_mouse_point = new CPoint(point);
}


void Cwaypoint_editorDlg::OnMButtonUp(UINT nFlags, CPoint point) {
	// TODO:  �ڴ������Ϣ�����������/�����Ĭ��ֵ

	CDialogEx::OnMButtonUp(nFlags, point);
	m_mouse_state = true;
	if (m_mouse_point) {
		delete m_mouse_point;
		m_mouse_point = NULL;
	}
}

void Cwaypoint_editorDlg::OnBnClickedButton1() {
	// TODO:  �ڴ���ӿؼ�֪ͨ����������
	CString nav_tile;
	nav_tile.Format(_T("./wp/%s.wp"), AfxGetApp()->m_lpCmdLine);

	std::ofstream ofs(nav_tile.GetBuffer(0), std::ios::binary);

	uint32_t wp_size = m_wp_list.size();
	ofs.write((char*)&wp_size, sizeof(wp_size));

	uint32_t countor = 0;
	std::map<uint32_t, uint32_t> id_map;
	std::map<int, WayPoint*>::iterator it = m_wp_list.begin();
	for (; it != m_wp_list.end(); it++) {
		WayPoint* wp = it->second;
		id_map[wp->id] = countor++;

		uint32_t val = wp->pt.x;
		ofs.write((char*)&val, sizeof(val));

		val = wp->pt.y;
		ofs.write((char*)&val, sizeof(val));
	}

	it = m_wp_list.begin();
	for (; it != m_wp_list.end(); it++) {
		WayPoint* wp = it->second;

		uint32_t val = wp->link.size();
		ofs.write((char*)&val, sizeof(val));

		std::set<int>::iterator it_set = wp->link.begin();
		for (; it_set != wp->link.end(); it_set++) {
			int wpid = *it_set;
			uint32_t id = id_map[wpid];
			val = id;
			ofs.write((char*)&val, sizeof(val));
		}
	}
	ofs.close();

	MessageBoxW(nav_tile, _T("��ʾ"), MB_OK);
}

void Cwaypoint_editorDlg::OnBnClickedButton2() {
	// TODO:  �ڴ���ӿؼ�֪ͨ����������

	char current_path[MAX_PATH];
	int path_pos = GetCurrentDirectoryA(MAX_PATH, current_path);
	CString path;
	path.Format(_T("%s/wp"), current_path);

	TCHAR szFilter[] = _T("�ı��ļ�(*.wp)|*.wp|�����ļ�(*.*)|*.*||");
	// ������ļ��Ի���   
	CFileDialog file_dlg(TRUE, _T("txt"), NULL, 0, szFilter, this);
	file_dlg.m_ofn.lpstrInitialDir = path;
	CString file_path;

	// ��ʾ���ļ��Ի���   
	if (IDOK == file_dlg.DoModal()) {
		// ���������ļ��Ի����ϵġ��򿪡���ť����ѡ����ļ�·����ʾ���༭����   
		CString file_path = file_dlg.GetPathName();
		FILE* fp = _wfopen(file_path.GetBuffer(0), _T("rb"));

		USES_CONVERSION;
		char * pFileName = T2A(file_path);

		m_last_wp = -1;
		m_wp_list.clear();

		MemoryStream ms;
		Util::ReadFile(pFileName, ms);

		uint32_t wp_size;
		ms >> wp_size;

		for (uint32_t i = 0; i < wp_size; ++i) {
			uint32_t x, z;
			ms >> x >> z;
			WayPoint* wp = new WayPoint();
			wp->id = i;
			wp->pt.x = x;
			wp->pt.y = z;
			wp->r = 250;
			wp->check = false;
			m_wp_list[wp->id] = wp;
		}

		for (uint32_t i = 0; i < wp_size; ++i) {
			WayPoint* wp = m_wp_list[i];
			uint32_t link_size;
			ms >> link_size;

			for (uint32_t j = 0; j < link_size; ++j) {
				uint32_t link_id;
				ms >> link_id;
				wp->link.insert(link_id);
			}
		}

		m_id_countor = wp_size;

		fclose(fp);

		Invalidate();
	}
}
