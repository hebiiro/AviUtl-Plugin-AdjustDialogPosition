#include "pch.h"

//--------------------------------------------------------------------

// デバッグ用コールバック関数。デバッグメッセージを出力する。
void ___outputLog(LPCTSTR text, LPCTSTR output)
{
	::OutputDebugString(output);
}

//--------------------------------------------------------------------

DLGPROC true_dialogProc = 0;
INT_PTR CALLBACK hook_dialogProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	DLGPROC dlgProc = (DLGPROC)::GetProp(hwnd, _T("AdjustDialogPosition"));

	if (!dlgProc)
		::SetProp(hwnd, _T("AdjustDialogPosition"), dlgProc = true_dialogProc);

	switch (message)
	{
	case WM_INITDIALOG:
		{
			// デフォルト処理を先に実行する。
			INT_PTR result = dlgProc(hwnd, message, wParam, lParam);

			// モニタ情報を取得する。
			HMONITOR monitor = ::MonitorFromWindow(hwnd, MONITOR_DEFAULTTONEAREST);
			MONITORINFO mi = { sizeof(mi) };
			::GetMonitorInfo(monitor, &mi);

			// ウィンドウ矩形を取得する。
			RECT rc; ::GetWindowRect(hwnd, &rc);

			// ウィンドウ矩形がモニタの範囲外なら
			if (rc.bottom > mi.rcWork.bottom)
			{
				// モニタ内に収まるようにウィンドウ矩形を調整する。

				::OffsetRect(&rc, 0, mi.rcWork.bottom - rc.bottom);

				int x = rc.left;
				int y = rc.top;
				int w = rc.right - rc.left;
				int h = rc.bottom - rc.top;

				::SetWindowPos(hwnd, 0, x, y, w, h, SWP_NOZORDER);
			}

			return result;
		}
	}

	return dlgProc(hwnd, message, wParam, lParam);
}

DECLARE_HOOK_PROC(INT_PTR, WINAPI, DialogBoxParamA, (HINSTANCE instance, LPCSTR templateName, HWND parent, DLGPROC dialogProc, LPARAM initParam));
IMPLEMENT_HOOK_PROC(INT_PTR, WINAPI, DialogBoxParamA, (HINSTANCE instance, LPCSTR templateName, HWND parent, DLGPROC dialogProc, LPARAM initParam))
{
	if ((DWORD)templateName <= 0x0000FFFF)
		MY_TRACE(_T("DialogBoxParamA(0x%08X, 0x%08X, 0x%08X, 0x%08X, 0x%08X)\n"), instance, templateName, parent, dialogProc, initParam);
	else
		MY_TRACE(_T("DialogBoxParamA(0x%08X, %s, 0x%08X, 0x%08X, 0x%08X)\n"), instance, templateName, parent, dialogProc, initParam);

//	if (::lstrcmpiA(templateName, "GET_SCENE") == 0)
	{
		// ダイアログ関数をフックする。
		true_dialogProc = dialogProc;
		dialogProc = hook_dialogProc;
	}

	return true_DialogBoxParamA(instance, templateName, parent, dialogProc, initParam);
}

//--------------------------------------------------------------------

BOOL func_init(AviUtl::FilterPlugin* fp)
{
	DetourTransactionBegin();
	DetourUpdateThread(::GetCurrentThread());

	ATTACH_HOOK_PROC(DialogBoxParamA);

	if (DetourTransactionCommit() == NO_ERROR)
	{
		MY_TRACE(_T("API フックに成功しました\n"));
	}
	else
	{
		MY_TRACE(_T("API フックに失敗しました\n"));
	}

	return TRUE;
}

BOOL func_exit(AviUtl::FilterPlugin* fp)
{
	DETACH_HOOK_PROC(DialogBoxParamA);

	return FALSE;
}

EXTERN_C AviUtl::FilterPluginDLL* CALLBACK GetFilterTable()
{
	LPCSTR name = "ダイアログ位置調整";
	LPCSTR information = "ダイアログ位置調整 1.0.1 by 蛇色";

	static AviUtl::FilterPluginDLL filter =
	{
		.flag =
			AviUtl::detail::FilterPluginFlag::NoConfig |
			AviUtl::detail::FilterPluginFlag::AlwaysActive |
			AviUtl::detail::FilterPluginFlag::DispFilter |
			AviUtl::detail::FilterPluginFlag::ExInformation,
		.name = name,
		.func_init = func_init,
		.func_exit = func_exit,
		.information = information,
	};

	return &filter;
}

//--------------------------------------------------------------------
