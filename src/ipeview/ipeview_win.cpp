// --------------------------------------------------------------------
// Ipeview with Windows UI
// --------------------------------------------------------------------
/*

    This file is part of the extensible drawing editor Ipe.
    Copyright (C) 1993-2013  Otfried Cheong

    Ipe is free software; you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    As a special exception, you have permission to link Ipe with the
    CGAL library and distribute executables, as long as you follow the
    requirements of the Gnu General Public License in regard to all of
    the software in the executable aside from CGAL.

    Ipe is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public
    License for more details.

    You should have received a copy of the GNU General Public License
    along with Ipe; if not, you can find it at
    "http://www.gnu.org/copyleft/gpl.html", or write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

*/

#include "ipedoc.h"

#include "ipecanvas_win.h"
#include "ipetool.h"

#include <windows.h>
#include <stdio.h>

#define IDI_MYICON 1

using namespace ipe;

// --------------------------------------------------------------------

class AppUi : public CanvasObserver {
public:
  enum TAction { EOpen = 9001, EQuit, EGridVisible, EFullScreen, EFitPage,
		 EZoomIn, EZoomOut, ENextView, EPreviousView,
		 ENumActions = EPreviousView - 9000 };

  static void init(HINSTANCE hInstance);

  AppUi(HINSTANCE hInstance);
  void show(int nCmdShow);

  bool load(const char* fn);

  void zoom(int delta);
  void nextView(int delta);
  void fitBox(const Rect &box);
  void updateLabel();

  void cmd(int cmd);

protected: // from CanvasObserver
  virtual void canvasObserverWheelMoved(int degrees);
  virtual void canvasObserverMouseAction(int button);

private:
  void initUi();
  ~AppUi();

  static LRESULT CALLBACK wndProc(HWND hwnd, UINT Message,
				  WPARAM wParam, LPARAM lParam);
  static const char className[];

  HWND hwnd;

  Canvas *iCanvas;
  Snap iSnap;

  bool iGridVisible;
  Document *iDoc;
  String iFileName;
  int iPageNo;
  int iViewNo;
};

const char AppUi::className[] = "ipeviewWindowClass";

AppUi::AppUi(HINSTANCE hInstance)
{
  iCanvas = 0;
  iDoc = 0;
  iGridVisible = false;
  iPageNo = iViewNo = 0;

  iSnap.iSnap = Snap::ESnapGrid | Snap::ESnapVtx;
  iSnap.iGridVisible = false;
  iSnap.iGridSize = 8;
  iSnap.iAngleSize = M_PI / 6.0;
  iSnap.iSnapDistance = 10;
  iSnap.iWithAxes = false;
  iSnap.iOrigin = Vector::ZERO;
  iSnap.iDir = 0;

  HWND hwnd = CreateWindowEx(WS_EX_CLIENTEDGE, className, "Ipeview",
			     WS_OVERLAPPEDWINDOW,
			     CW_USEDEFAULT, CW_USEDEFAULT,
			     CW_USEDEFAULT, CW_USEDEFAULT,
			     NULL, NULL, hInstance, this);

  if (hwnd == NULL) {
    MessageBox(NULL, "AppUi window creation failed!", "Error!",
	       MB_ICONEXCLAMATION | MB_OK);
    exit(9);
  }
  assert(GetWindowLongPtr(hwnd, GWLP_USERDATA));
}

void AppUi::initUi()
{
  HMENU hMenu = CreateMenu();

  HMENU hSubMenu = CreatePopupMenu();
  AppendMenu(hSubMenu, MF_STRING, EOpen, "&Open\tCtrl+O");
  AppendMenu(hSubMenu, MF_STRING, EQuit, "&Quit\tCtrl+Q");
  AppendMenu(hMenu, MF_STRING | MF_POPUP, UINT(hSubMenu), "&File");

  hSubMenu = CreatePopupMenu();
  AppendMenu(hSubMenu, MF_STRING, EGridVisible, "&Grid visible\tF12");
  AppendMenu(hSubMenu, MF_STRING, EFitPage, "&Fit page\tCtrl+F");
  AppendMenu(hSubMenu, MF_STRING, EZoomIn, "Zoom &in\tCtrl++");
  AppendMenu(hSubMenu, MF_STRING, EZoomOut, "Zoom &out\tCtrl+-");
  AppendMenu(hMenu, MF_STRING | MF_POPUP, UINT(hSubMenu), "&View");

  hSubMenu = CreatePopupMenu();
  AppendMenu(hSubMenu, MF_STRING, ENextView, "&Next view\tPageDn");
  AppendMenu(hSubMenu, MF_STRING, EPreviousView, "&Previous view\tPageUp");
  AppendMenu(hMenu, MF_STRING | MF_POPUP, UINT(hSubMenu), "&Move");

  SetMenu(hwnd, hMenu);

  iCanvas = new Canvas(hwnd);
  iCanvas->setSnap(iSnap);
  iCanvas->setObserver(this);
  // SetWindowPos(hwnd, HWND_NOTOPMOST, -20, -20, 1000, 800, SWP_SHOWWINDOW);
}

AppUi::~AppUi()
{
  fprintf(stderr, "AppUi::~AppUi()\n");
}

// --------------------------------------------------------------------

void AppUi::cmd(int cmd)
{
  // ipeDebug("Command %d", cmd);
  switch (cmd) {
  case EOpen:
    {
      OPENFILENAME ofn;
      char szFileName[MAX_PATH] = "";
      ZeroMemory(&ofn, sizeof(ofn));

      ofn.lStructSize = sizeof(ofn); // SEE NOTE BELOW
      ofn.hwndOwner = hwnd;
      ofn.lpstrFilter =
	"Ipe Files\0*.ipe;*.pdf;*.eps\0All Files\0*.*\0";
      ofn.lpstrFile = szFileName;
      ofn.nMaxFile = MAX_PATH;
      ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY;
      ofn.lpstrDefExt = "ipe";

      if (GetOpenFileName(&ofn))
	load(szFileName);
    }
    break;
  case EQuit:
    PostMessage(hwnd, WM_CLOSE, 0, 0);
    break;
  case EGridVisible: {
    Snap snap = iCanvas->snap();
    snap.iGridVisible = !snap.iGridVisible;
    iCanvas->setSnap(snap);
    CheckMenuItem(GetMenu(hwnd), EGridVisible, snap.iGridVisible ?
		  MF_CHECKED : MF_UNCHECKED);
    iCanvas->update();
    break; }
  case EFitPage:
    fitBox(iDoc->cascade()->findLayout()->paper());
    break;
  case EZoomIn:
   zoom(+1);
    break;
  case EZoomOut:
    zoom(-1);
    break;
  case ENextView:
    nextView(+1);
    break;
  case EPreviousView:
    nextView(-1);
    break;
  default:
    // unknown action
    return;
  }
}

void AppUi::canvasObserverWheelMoved(int degrees)
{
  if (degrees > 0)
    zoom(+1);
  else
    zoom(-1);
}

void AppUi::canvasObserverMouseAction(int button)
{
  Tool *tool = new PanTool(iCanvas, iDoc->page(iPageNo), iViewNo);
  iCanvas->setTool(tool);
}

// --------------------------------------------------------------------

bool AppUi::load(const char *fname)
{
  Document *doc = Document::loadWithErrorReport(fname);
  if (!doc)
    return false;

  iFileName = String(fname);
  doc->runLatex();

  delete iDoc;
  iDoc = doc;
  iPageNo = 0;
  iViewNo = 0;

  /*
  if (iDoc->countPages() > 1) {
    int p = PageSelector::selectPageOrView(iDoc);
    if (p >= 0)
      iPageNo = p;
    if (iDoc->page(iPageNo)->countViews() > 1) {
      int v = PageSelector::selectPageOrView(iDoc, iPageNo);
      if (v >= 0)
	iViewNo = v;
    }
  }
  */

  // iDoc->page(iPageNo)->setSelect(0, EPrimarySelected);

  iCanvas->setFontPool(iDoc->fontPool());
  iCanvas->setPage(iDoc->page(iPageNo), iPageNo, iViewNo, iDoc->cascade());
  iCanvas->setPan(Vector(300, 400));
  iCanvas->update();
  iCanvas->setFifiVisible(true);

  updateLabel();
  return true;
}

// --------------------------------------------------------------------

void AppUi::updateLabel()
{
  String s = iFileName;
  if (iFileName.rfind('/') >= 0)
    s = iFileName.substr(iFileName.rfind('/') + 1);
  if (iDoc->countTotalViews() > 1) {
    ipe::StringStream ss(s);
    ss << " (" << iPageNo + 1 << "-" << iViewNo + 1 << ")";
  }
  SetWindowText(hwnd, s.z());
}

void AppUi::fitBox(const Rect &box)
{
  if (box.isEmpty())
    return;
  double xfactor = box.width() > 0.0 ?
    (iCanvas->canvasWidth() / box.width()) : 20.0;
  double yfactor = box.height() > 0.0 ?
    (iCanvas->canvasHeight() / box.height()) : 20.0;
  double zoom = (xfactor > yfactor) ? yfactor : xfactor;
  iCanvas->setPan(0.5 * (box.bottomLeft() + box.topRight()));
  iCanvas->setZoom(zoom);
  iCanvas->update();
}

void AppUi::zoom(int delta)
{
  double zoom = iCanvas->zoom();
  while (delta > 0) {zoom *= 1.3; --delta;}
  while (delta < 0) {zoom /= 1.3; ++delta;}
  iCanvas->setZoom(zoom);
  iCanvas->update();
}

void AppUi::nextView(int delta)
{
  const Page *page = iDoc->page(iPageNo);
  if (0 <= iViewNo + delta && iViewNo + delta < page->countViews()) {
    iViewNo += delta;
  } else if (0 <= iPageNo + delta && iPageNo + delta < iDoc->countPages()) {
    iPageNo += delta;
    if (delta > 0)
      iViewNo = 0;
    else
      iViewNo = iDoc->page(iPageNo)->countViews() - 1;
  } else
    // at beginning or end of sequence
    return;
  iCanvas->setPage(iDoc->page(iPageNo), iPageNo, iViewNo, iDoc->cascade());
  iCanvas->update();
  updateLabel();
}

// --------------------------------------------------------------------

LRESULT CALLBACK AppUi::wndProc(HWND hwnd, UINT message, WPARAM wParam,
				LPARAM lParam)
{
  AppUi *ui = (AppUi*) GetWindowLongPtr(hwnd, GWLP_USERDATA);
  switch (message) {
  case WM_CREATE:
    {
      LPCREATESTRUCT p = (LPCREATESTRUCT) lParam;
      ui = (AppUi *) p->lpCreateParams;
      ui->hwnd = hwnd;
      SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR) ui);
      ui->initUi();
    }
    break;
    /*
  case WM_INITMENUPOPUP:
    if (ui && lParam == 1) {
      CheckMenuItem(EGridVisible);
    }
    break;
    */
  case WM_COMMAND:
    if (ui)
      ui->cmd(LOWORD(wParam));
    break;
  case WM_SIZE:
    if (ui && ui->iCanvas) {
      RECT rc;
      GetClientRect(hwnd, &rc);
      MoveWindow(ui->iCanvas->windowId(), 0, 0, rc.right, rc.bottom, TRUE);
    }
    break;
  case WM_CLOSE:
    DestroyWindow(hwnd);
    break;
  case WM_DESTROY:
    PostQuitMessage(0);
    delete ui;
    break;
  default:
    break;
  }
  return DefWindowProc(hwnd, message, wParam, lParam);
}

void AppUi::init(HINSTANCE hInstance)
{
  WNDCLASSEX wc;
  wc.cbSize = sizeof(WNDCLASSEX);
  wc.style = 0;
  wc.lpfnWndProc = wndProc;
  wc.cbClsExtra	 = 0;
  wc.cbWndExtra	 = 0;
  wc.hInstance	 = hInstance;
  wc.hCursor = LoadCursor(NULL, IDC_ARROW);
  wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
  wc.lpszMenuName = NULL;
  wc.lpszClassName = className;
  wc.hIcon = LoadIcon(GetModuleHandle(NULL), MAKEINTRESOURCE(IDI_MYICON));
  wc.hIconSm =
    HICON(LoadImage(GetModuleHandle(NULL),
		    MAKEINTRESOURCE(IDI_MYICON), IMAGE_ICON, 16, 16, 0));

  if (!RegisterClassEx(&wc)) {
    MessageBox(NULL, "AppUi registration failed!", "Error!",
	       MB_ICONEXCLAMATION | MB_OK);
    exit(9);
  }
}

void AppUi::show(int nCmdShow)
{
  ShowWindow(hwnd, nCmdShow);
  UpdateWindow(hwnd);
}

// --------------------------------------------------------------------

void usage()
{
  MessageBox(NULL, "Ipeview needs to be called with a filename argument!",
	     "Ipeview Error!",
	     MB_ICONEXCLAMATION | MB_OK);
  exit(9);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
		   LPSTR lpCmdLine, int nCmdShow)
{
  Platform::initLib(IPELIB_VERSION);

  AppUi::init(hInstance);
  ipe_init_canvas(hInstance);

  AppUi *ui = new AppUi(hInstance);

  if (strlen(lpCmdLine) == 0)
    usage();

  if (!ui->load(lpCmdLine))
    usage();

  ui->show(nCmdShow);

  ACCEL accel[] = {
    { FVIRTKEY|FCONTROL, 'O', AppUi::EOpen },
    { FVIRTKEY|FCONTROL, 'F', AppUi::EFitPage },
    { FVIRTKEY|FCONTROL, 'Q', AppUi::EQuit },
    { FVIRTKEY, VK_PRIOR, AppUi::EPreviousView },
    { FVIRTKEY, VK_NEXT, AppUi::ENextView },
    { FVIRTKEY|FCONTROL|FSHIFT, 0xbb, AppUi::EZoomIn },
    { FVIRTKEY|FCONTROL, 0xbd, AppUi::EZoomOut },
    { FVIRTKEY, VK_F12, AppUi::EGridVisible },
  };
  HACCEL hAccel = CreateAcceleratorTable(accel, sizeof(accel)/sizeof(ACCEL));

  MSG msg;
  while (GetMessage(&msg, NULL, 0, 0) > 0) {
    if (!TranslateAccelerator(msg.hwnd, hAccel, &msg)) {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
    }
  }
  return msg.wParam;
}

// --------------------------------------------------------------------
