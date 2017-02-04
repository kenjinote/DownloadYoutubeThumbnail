#pragma comment(linker,"\"/manifestdependency:type='win32' name='Microsoft.Windows.Common-Controls' version='6.0.0.0' processorArchitecture='*' publicKeyToken='6595b64144ccf1df' language='*'\"")

#pragma comment(lib,"wininet")
#pragma comment(lib,"Shlwapi")
#pragma comment(lib,"gdiplus")

#include <windows.h>
#include <wininet.h>
#include <shlwapi.h>
#include <gdiplus.h>

TCHAR szClassName[] = TEXT("Window");

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
			++pEnd;
			lstrcpyn(lpszID, pStart, (int)(pEnd - pStart));
			return TRUE;
		}
	}
	return FALSE;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	static Gdiplus::Image*pImage;
	static HWND hEdit;
	static HWND hButton;
	switch (msg)
	{
	case WM_CREATE:
		hEdit = CreateWindowEx(WS_EX_CLIENTEDGE, TEXT("EDIT"), 0, WS_VISIBLE | WS_CHILD | WS_TABSTOP | ES_AUTOHSCROLL, 0, 0, 0, 0, hWnd, 0, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		hButton = CreateWindow(TEXT("BUTTON"), TEXT("取得"), WS_VISIBLE | WS_CHILD | WS_TABSTOP | BS_DEFPUSHBUTTON, 0, 0, 0, 0, hWnd, (HMENU)IDOK, ((LPCREATESTRUCT)lParam)->hInstance, 0);
		break;
	case WM_SIZE:
		MoveWindow(hEdit, 10, 10, LOWORD(lParam) - 20 - 128 , 32, TRUE);
		MoveWindow(hButton, LOWORD(lParam) - 138, 10, 128, 32, TRUE);
		break;
	case WM_COMMAND:
		if (LOWORD(wParam) == IDOK)
		{
			TCHAR szURL[1024], szID[16];
			GetWindowText(hEdit, szURL, 1024);
			if (lstrlen(szURL) > 0 && GetVideoID(szURL, szID))
			{
				lstrcpy(szURL, TEXT("http://i.ytimg.com/vi/"));
				lstrcat(szURL, szID);
				lstrcat(szURL, TEXT("/maxresdefault.jpg"));
				LPBYTE lpByte = DownloadToMemory(szURL);
				if (lpByte)
				{
					const SIZE_T size = (int)GlobalSize(lpByte);
					pImage = LoadImageFromMemory(lpByte, size);
					GlobalFree(lpByte);
					InvalidateRect(hWnd, 0, 0);
				}
			}
		}
		break;
	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(hWnd, &ps);
			RECT rect;
			GetClientRect(hWnd, &rect);
			if (pImage)
			{
				int x, y, width, height;
				if ((double)rect.right / (double)rect.bottom < (double)pImage->GetWidth() / (double)pImage->GetHeight())
				{
					width = rect.right;
					height = (int)((double)pImage->GetHeight() * ((double)rect.right / (double)pImage->GetWidth()));
					x = 0;
					y = (rect.bottom - height) / 2;
					RECT rect1 = { 0, 0, rect.right, y };
					RECT rect2 = { 0, y + height, rect.right, rect.bottom };
					FillRect(hdc, &rect1, (HBRUSH)GetStockObject(WHITE_BRUSH));
					FillRect(hdc, &rect2, (HBRUSH)GetStockObject(WHITE_BRUSH));
				}
				else
				{
					width = (int)((double)pImage->GetWidth() * ((double)rect.bottom / (double)pImage->GetHeight()));
					height = rect.bottom;
					x = (rect.right - width) / 2;
					y = 0;
					RECT rect1 = { 0, 0, x, rect.bottom };
					RECT rect2 = { x + width, 0, rect.right, rect.bottom };
					FillRect(hdc, &rect1, (HBRUSH)GetStockObject(WHITE_BRUSH));
					FillRect(hdc, &rect2, (HBRUSH)GetStockObject(WHITE_BRUSH));
				}
				{
					Gdiplus::Graphics graphics(hdc);
					graphics.DrawImage(pImage, x, y, width, height);
				}
			}
			else
			{
				FillRect(hdc, &rect, (HBRUSH)GetStockObject(WHITE_BRUSH));
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
	WNDCLASS wndclass = {
		CS_HREDRAW | CS_VREDRAW,
		WndProc,
		0,
		DLGWINDOWEXTRA,
		hInstance,
		0,
		LoadCursor(0,IDC_ARROW),
		0,
		0,
		szClassName
	};
	RegisterClass(&wndclass);
	HWND hWnd = CreateWindow(
		szClassName,
		TEXT("YouTubeの動画URLからサムネイルを取得する"),
		WS_OVERLAPPEDWINDOW | WS_CLIPCHILDREN,
		CW_USEDEFAULT,
		0,
		CW_USEDEFAULT,
		0,
		0,
		0,
		hInstance,
		0
	);
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
