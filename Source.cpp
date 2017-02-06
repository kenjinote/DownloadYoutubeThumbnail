#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma comment(lib,"wininet")
#pragma comment(lib,"Shlwapi")
#pragma comment(lib,"gdiplus")
#pragma comment (lib, "shlwapi.lib")
#pragma comment (lib, "comctl32.lib")

#include <windows.h>
#include <wininet.h>
#include <shlwapi.h>
#include <gdiplus.h>

#include <shlobj.h>
#include <shlwapi.h>

#define DRAG_IMAGE_WIDTH (1280/4)
#define DRAG_IMAGE_HEIGHT (720/4)

TCHAR szClassName[] = TEXT("Window");

BOOL SetDragImage(IN HWND hWnd, IN IDragSourceHelper *pDragSourceHelper, IN IDataObject *pDataObject, IN Gdiplus::Image *image, IN LPPOINT pt)
{
	SIZE size = { DRAG_IMAGE_WIDTH, DRAG_IMAGE_HEIGHT };
	HDC hdc = GetDC(hWnd);
	HDC hdcMem = CreateCompatibleDC(hdc);
	HBITMAP hbmp = CreateCompatibleBitmap(hdc, size.cx, size.cy);
	HBITMAP hbmpPrev = (HBITMAP)SelectObject(hdcMem, hbmp);
	{
		Gdiplus::Graphics g(hdcMem);
		g.DrawImage(image, 0, 0, size.cx, size.cy);
	}
	SelectObject(hdcMem, hbmpPrev);
	DeleteDC(hdcMem);
	ReleaseDC(hWnd, hdc);
	SHDRAGIMAGE dragImage;
	dragImage.sizeDragImage = size;
	dragImage.ptOffset = *pt;
	dragImage.hbmpDragImage = hbmp;
	dragImage.crColorKey = RGB(0, 0, 0);
	HRESULT hr = pDragSourceHelper->InitializeFromBitmap(&dragImage, pDataObject);
	return hr == S_OK;
}

class CDropTarget : public IDropTarget
{
public:
	CDropTarget(HWND hwnd)
	{
		m_cRef = 1;
		m_hwnd = hwnd;
		CoCreateInstance(CLSID_DragDropHelper, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&m_pDropTargetHelper));
	}
	~CDropTarget()
	{
		if (m_pDropTargetHelper != NULL)
			m_pDropTargetHelper->Release();
	}
	STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject)
	{
		*ppvObject = NULL;
		if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IDropTarget))
			*ppvObject = static_cast<IDropTarget *>(this);
		else
			return E_NOINTERFACE;
		AddRef();
		return S_OK;
	}
	STDMETHODIMP_(ULONG) AddRef()
	{
		return InterlockedIncrement(&m_cRef);
	}
	STDMETHODIMP_(ULONG) Release()
	{
		if (InterlockedDecrement(&m_cRef) == 0)
		{
			delete this;
			return 0;
		}
		return m_cRef;
	}
	STDMETHODIMP DragEnter(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
	{
		if (m_pDropTargetHelper != NULL)
			m_pDropTargetHelper->DragEnter(m_hwnd, pDataObj, (LPPOINT)&pt, *pdwEffect);
		return S_OK;
	}
	STDMETHODIMP DragOver(DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
	{
		if (m_pDropTargetHelper != NULL)
			m_pDropTargetHelper->DragOver((LPPOINT)&pt, *pdwEffect);
		return S_OK;
	}
	STDMETHODIMP DragLeave()
	{
		if (m_pDropTargetHelper != NULL)
			m_pDropTargetHelper->DragLeave();
		return S_OK;
	}
	STDMETHODIMP Drop(IDataObject *pDataObj, DWORD grfKeyState, POINTL pt, DWORD *pdwEffect)
	{
		if (m_pDropTargetHelper != NULL)
			m_pDropTargetHelper->Drop(pDataObj, (LPPOINT)&pt, *pdwEffect);
		return S_OK;
	}
private:
	LONG              m_cRef;
	HWND              m_hwnd;
	IDropTargetHelper *m_pDropTargetHelper;
};

class CDropSource : public IDropSource
{
public:
	CDropSource() { m_cRef = 1; }
	~CDropSource() {}
	STDMETHODIMP QueryInterface(REFIID riid, void **ppvObject)
	{
		*ppvObject = NULL;
		if (IsEqualIID(riid, IID_IUnknown) || IsEqualIID(riid, IID_IDropSource))
			*ppvObject = static_cast<IDropSource *>(this);
		else
			return E_NOINTERFACE;
		AddRef();
		return S_OK;
	}
	STDMETHODIMP_(ULONG) AddRef()
	{
		return InterlockedIncrement(&m_cRef);
	}
	STDMETHODIMP_(ULONG) Release()
	{
		if (InterlockedDecrement(&m_cRef) == 0)
		{
			delete this;
			return 0;
		}
		return m_cRef;
	}
	STDMETHODIMP QueryContinueDrag(BOOL fEscapePressed, DWORD grfKeyState)
	{
		if (fEscapePressed)
			return DRAGDROP_S_CANCEL;
		if ((grfKeyState & MK_LBUTTON) == 0)
			return DRAGDROP_S_DROP;
		return S_OK;
	}
	STDMETHODIMP GiveFeedback(DWORD dwEffect)
	{
		return DRAGDROP_S_USEDEFAULTCURSORS;
	}
private:
	LONG m_cRef;
};

Gdiplus::Image* LoadImageFromMemory(const void* pData, const SIZE_T nSize)
{
	IStream*pIStream;
	ULARGE_INTEGER LargeUInt;
	LARGE_INTEGER LargeInt;
	Gdiplus::Image*pImage;
	CreateStreamOnHGlobal(0, 1, &pIStream);
	LargeUInt.QuadPart = nSize;
	pIStream->SetSize(LargeUInt);
	LargeInt.QuadPart = 0;
	pIStream->Seek(LargeInt, STREAM_SEEK_SET, NULL);
	pIStream->Write(pData, (ULONG)nSize, NULL);
	pImage = Gdiplus::Image::FromStream(pIStream);
	pIStream->Release();
	if (pImage)
	{
		if (pImage->GetLastStatus() == Gdiplus::Ok)
		{
			return pImage;
		}
		delete pImage;
	}
	return 0;
}

LPBYTE DownloadToMemory(IN LPCWSTR lpszURL)
{
	LPBYTE lpszReturn = 0;
	DWORD dwSize = 0;
	const HINTERNET hSession = InternetOpenW(L"Mozilla/5.0 (Windows NT 6.3; WOW64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/38.0.2125.111 Safari/537.36", INTERNET_OPEN_TYPE_PRECONFIG, 0, 0, INTERNET_FLAG_NO_COOKIES);
	if (hSession)
	{
		URL_COMPONENTSW uc = { 0 };
		WCHAR HostName[MAX_PATH];
		WCHAR UrlPath[MAX_PATH];
		uc.dwStructSize = sizeof(uc);
		uc.lpszHostName = HostName;
		uc.lpszUrlPath = UrlPath;
		uc.dwHostNameLength = MAX_PATH;
		uc.dwUrlPathLength = MAX_PATH;
		InternetCrackUrlW(lpszURL, 0, 0, &uc);
		const HINTERNET hConnection = InternetConnectW(hSession, HostName, INTERNET_DEFAULT_HTTP_PORT, 0, 0, INTERNET_SERVICE_HTTP, 0, 0);
		if (hConnection)
		{
			const HINTERNET hRequest = HttpOpenRequestW(hConnection, L"GET", UrlPath, 0, 0, 0, 0, 0);
			if (hRequest)
			{
				ZeroMemory(&uc, sizeof(URL_COMPONENTS));
				WCHAR Scheme[16];
				uc.dwStructSize = sizeof(uc);
				uc.lpszScheme = Scheme;
				uc.lpszHostName = HostName;
				uc.dwSchemeLength = 16;
				uc.dwHostNameLength = MAX_PATH;
				InternetCrackUrlW(lpszURL, 0, 0, &uc);
				WCHAR szReferer[1024];
				lstrcpyW(szReferer, L"Referer: ");
				lstrcatW(szReferer, Scheme);
				lstrcatW(szReferer, L"://");
				lstrcatW(szReferer, HostName);
				HttpAddRequestHeadersW(hRequest, szReferer, lstrlenW(szReferer), HTTP_ADDREQ_FLAG_REPLACE | HTTP_ADDREQ_FLAG_ADD);
				HttpSendRequestW(hRequest, 0, 0, 0, 0);
				lpszReturn = (LPBYTE)GlobalAlloc(GMEM_FIXED, 1);
				DWORD dwRead;
				static BYTE szBuf[1024 * 4];
				LPBYTE lpTmp;
				for (;;)
				{
					if (!InternetReadFile(hRequest, szBuf, (DWORD)sizeof(szBuf), &dwRead) || !dwRead) break;
					lpTmp = (LPBYTE)GlobalReAlloc(lpszReturn, (SIZE_T)(dwSize + dwRead), GMEM_MOVEABLE);
					if (lpTmp == NULL) break;
					lpszReturn = lpTmp;
					CopyMemory(lpszReturn + dwSize, szBuf, dwRead);
					dwSize += dwRead;
				}
				InternetCloseHandle(hRequest);
			}
			InternetCloseHandle(hConnection);
		}
		InternetCloseHandle(hSession);
	}
	return lpszReturn;
}

BOOL GetVideoID(LPCTSTR lpszURL, LPTSTR lpszID)
{
	URL_COMPONENTS uc = { sizeof(uc) };
	TCHAR ExtraInfo[256];
	uc.lpszExtraInfo = ExtraInfo;
	uc.dwExtraInfoLength = 256;
	InternetCrackUrl(lpszURL, 0, 0, &uc);
	LPTSTR pStart = 0, pEnd = 0;
	if ((pStart = StrStr(ExtraInfo, TEXT("v="))) != 0)
	{
		pStart += 2;
		if (*pStart != 0)
		{
			pEnd = StrStr(pStart, TEXT("&"));
			if (pEnd == NULL)
			{
				pEnd = ExtraInfo + lstrlen(ExtraInfo);
			}
			lstrcpyn(lpszID, pStart, (int)(pEnd - pStart + 1));
			return TRUE;
		}
	}
	return FALSE;
}

BOOL CreateTempDirectory(OUT LPTSTR pszDir)
{
	DWORD dwSize = GetTempPath(0, 0);
	if (dwSize == 0 || dwSize > MAX_PATH - 14) { return FALSE; }
	LPTSTR pTmpPath;
	pTmpPath = (LPTSTR)GlobalAlloc(GPTR, sizeof(TCHAR)*(dwSize + 1));
	GetTempPath(dwSize + 1, pTmpPath);
	dwSize = GetTempFileName(pTmpPath, TEXT(""), 0, pszDir);
	GlobalFree(pTmpPath);
	if (dwSize == 0) { return FALSE; }
	DeleteFile(pszDir);
	if (CreateDirectory(pszDir, 0) == 0) { return FALSE; }
	return TRUE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static Gdiplus::Image*pImage;
	static HWND hEdit, hButton;
	static IDropTarget*pDropTarget;
	static IDropSource*pDropSource;
	static IDragSourceHelper*pDragSourceHelper;
	static TCHAR szTempDirectory[MAX_PATH];
	static TCHAR szImageFilePath[MAX_PATH];
	switch (msg)
	{
	case WM_CREATE:
		{
			OleInitialize(NULL);
			pDropTarget = new CDropTarget(hWnd);
			pDropSource = new CDropSource();
			CoCreateInstance(CLSID_DragDropHelper, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pDragSourceHelper));
			RegisterDragDrop(hWnd, pDropTarget);
			hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("EDIT"), 0, WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL, 0, 0, 0, 0, hWnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, 0);
			hButton = CreateWindow(TEXT("BUTTON"), TEXT("取得"), WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_DEFPUSHBUTTON, 0, 0, 0, 0, hWnd, (HMENU)IDOK, ((LPCREATESTRUCT)lParam)->hInstance, 0);
			if (CreateTempDirectory(szTempDirectory) == FALSE) return -1;
		}
		break;
	case WM_SIZE:
		MoveWindow(hEdit, 10, 10, LOWORD(lParam) - 20 - 128, 32, TRUE);
		MoveWindow(hButton, LOWORD(lParam) - 138, 10, 128, 32, TRUE);
		break;
	case WM_LBUTTONDOWN:
		if (pImage)
		{
			PIDLIST_ABSOLUTE*ppidlAbsolute = (PIDLIST_ABSOLUTE *)CoTaskMemAlloc(sizeof(PIDLIST_ABSOLUTE));
			PITEMID_CHILD*ppidlChild = (PITEMID_CHILD *)CoTaskMemAlloc(sizeof(PITEMID_CHILD));
			ppidlAbsolute[0] = ILCreateFromPath(szImageFilePath);
			IShellFolder*pShellFolder = NULL;
			SHBindToParent(ppidlAbsolute[0], IID_PPV_ARGS(&pShellFolder), NULL);
			ppidlChild[0] = ILFindLastID(ppidlAbsolute[0]);
			IDataObject*pDataObject;
			pShellFolder->GetUIObjectOf(NULL, 1, (LPCITEMIDLIST *)ppidlChild, IID_IDataObject, NULL, (void **)&pDataObject);
			RECT rect;
			GetClientRect(hWnd, &rect);
			POINT point = { (LONG)(DRAG_IMAGE_WIDTH * LOWORD(lParam) / rect.right), (LONG)(DRAG_IMAGE_HEIGHT * HIWORD(lParam) / rect.bottom) };
			SetDragImage(hWnd, pDragSourceHelper, pDataObject, pImage, &point);
			DWORD dwEffect;
			DoDragDrop(pDataObject, pDropSource, DROPEFFECT_COPY, &dwEffect);
			CoTaskMemFree(ppidlAbsolute[0]);
			CoTaskMemFree(ppidlAbsolute);
			CoTaskMemFree(ppidlChild);
		}
		break;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			TCHAR szURL[1024], szID[16];
			GetWindowText(hEdit, szURL, 1024);
			if (lstrlen(szURL) > 0 && GetVideoID(szURL, szID))
			{
				LPCTSTR lpszFileName[] = { TEXT("maxresdefault.jpg"), TEXT("sddefault.jpg"), TEXT("hqdefault.jpg"), TEXT("mqdefault.jpg"), TEXT("default.jpg") };
				for (auto filename : lpszFileName)
				{
					wsprintf(szURL, TEXT("http://i.ytimg.com/vi/%s/%s"), szID, filename);
					const LPBYTE lpByte = DownloadToMemory(szURL);
					if (lpByte)
					{
						const SIZE_T size = (int)GlobalSize(lpByte);
						delete pImage;
						pImage = LoadImageFromMemory(lpByte, size);
						if (filename == lpszFileName[sizeof(lpszFileName) / sizeof(LPCTSTR) - 1] ||
							(pImage != 0 && (pImage->GetWidth() > 120 && pImage->GetWidth() > 90)))
						{
							if (PathFileExists(szImageFilePath))
							{
								DeleteFile(szImageFilePath);
							}
							lstrcpy(szImageFilePath, szTempDirectory);
							PathAppend(szImageFilePath, filename);
							const HANDLE hFile = CreateFile(szImageFilePath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
							DWORD dwWritten;
							WriteFile(hFile, lpByte, (DWORD)size, &dwWritten, NULL);
							CloseHandle(hFile);
							GlobalFree(lpByte);
							break;
						}
						else
						{
							GlobalFree(lpByte);
							delete pImage;
							pImage = NULL;
						}
					}
				}
				InvalidateRect(hWnd, 0, 0);
			}
		}
		break;
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		const HDC hdc = BeginPaint(hWnd, &ps);
		RECT rect;
		GetClientRect(hWnd, &rect);
		const HBRUSH hWhiteBrush = (HBRUSH)GetStockObject(WHITE_BRUSH);
		if (pImage)
		{
			int x, y, width, height;
			if ((double)rect.right / (double)rect.bottom < (double)pImage->GetWidth() / (double)pImage->GetHeight())
			{
				width = rect.right;
				height = (int)((double)pImage->GetHeight() * ((double)rect.right / (double)pImage->GetWidth()));
				x = 0;
				y = (rect.bottom - height) / 2;
				const RECT rect1 = { 0, 0, rect.right, y };
				const RECT rect2 = { 0, y + height, rect.right, rect.bottom };
				FillRect(hdc, &rect1, hWhiteBrush);
				FillRect(hdc, &rect2, hWhiteBrush);
			}
			else
			{
				width = (int)((double)pImage->GetWidth() * ((double)rect.bottom / (double)pImage->GetHeight()));
				height = rect.bottom;
				x = (rect.right - width) / 2;
				y = 0;
				const RECT rect1 = { 0, 0, x, rect.bottom };
				const RECT rect2 = { x + width, 0, rect.right, rect.bottom };
				FillRect(hdc, &rect1, hWhiteBrush);
				FillRect(hdc, &rect2, hWhiteBrush);
			}
			{
				Gdiplus::Graphics graphics(hdc);
				graphics.DrawImage(pImage, x, y, width, height);
			}
		}
		else
		{
			FillRect(hdc, &rect, hWhiteBrush);
		}
		EndPaint(hWnd, &ps);
	}
	break;
	case WM_ERASEBKGND:
		return 1;
	case WM_CLOSE:
		DestroyWindow(hWnd);
		break;
	case WM_DESTROY:
		delete pImage;
		if (pDropTarget != NULL) pDropTarget->Release();
		if (pDropSource != NULL) pDropSource->Release();
		if (pDragSourceHelper != NULL) pDragSourceHelper->Release();
		RevokeDragDrop(hWnd);
		OleUninitialize();
		if (PathFileExists(szImageFilePath)) DeleteFile(szImageFilePath);
		RemoveDirectory(szTempDirectory);
		PostQuitMessage(0);
		break;
	default:
		return DefDlgProc(hWnd, msg, wParam, lParam);
	}
	return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPreInst, LPSTR pCmdLine, int nCmdShow)
{
	ULONG_PTR gdiToken;
	Gdiplus::GdiplusStartupInput gdiSI;
	Gdiplus::GdiplusStartup(&gdiToken, &gdiSI, 0);
	MSG msg;
	const WNDCLASS wndclass = { CS_HREDRAW | CS_VREDRAW,WndProc,0,DLGWINDOWEXTRA,hInstance,0,LoadCursor(0,IDC_ARROW),0,0,szClassName };
	RegisterClass(&wndclass);
	const HWND hWnd = CreateWindow(szClassName, TEXT("YouTubeの動画URLからサムネイルを取得する"), WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN, CW_USEDEFAULT, 0, CW_USEDEFAULT, 0, 0, 0, hInstance, 0);
	ShowWindow(hWnd, SW_SHOWDEFAULT);
	UpdateWindow(hWnd);
	while (GetMessage(&msg, 0, 0, 0))
	{
		if (!IsDialogMessage(hWnd, &msg))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	Gdiplus::GdiplusShutdown(gdiToken);
	return (int)msg.wParam;
}