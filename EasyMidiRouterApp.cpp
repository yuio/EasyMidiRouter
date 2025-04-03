#include <windows.h>
#include <string>
#include <vector>
#include <thread>
#include <iostream>
#include <sstream>
#include <streambuf>
#include <mutex>
#include "resource.h"

extern int main(int argc, wchar_t* argv[]);

HWND g_hEdit = nullptr;
HFONT g_hFont = nullptr;
HANDLE g_hThread = nullptr;
std::mutex g_outputMutex;
const size_t g_maxChars=4000;

class EditStreamBuf : public std::wstreambuf {
protected:
    std::wstring buffer;
    const size_t flushThreshold = 64;

    int_type overflow(int_type ch) override {
        if (ch != EOF) {
            buffer += static_cast<wchar_t>(ch);
            if (ch == '\n' || buffer.size() >= flushThreshold) {
                flush();
            }
        }
        return ch;
    }

    int sync() override {
        flush();
        return 0;
    }

    void flush() 
    {
       if (buffer.empty() || !g_hEdit) return;

       std::lock_guard<std::mutex> lock(g_outputMutex);

       LRESULT textLength = SendMessageW(g_hEdit, WM_GETTEXTLENGTH, 0, 0);
       if (textLength > g_maxChars) {
           SendMessageW(g_hEdit, WM_SETTEXT, 0, (LPARAM)L"");
       }

        SendMessageW(g_hEdit, EM_SETSEL, -1, -1);
        SendMessageW(g_hEdit, EM_REPLACESEL, FALSE, (LPARAM)buffer.c_str());
        buffer.clear();
    }
};

void GuiRedirectThread() {
    EditStreamBuf editBuf;
    std::wostream editStream(&editBuf);
    std::wstreambuf* oldCoutBuf = std::wcout.rdbuf(&editBuf);
    std::wstreambuf* oldCerrBuf = std::wcerr.rdbuf(&editBuf);

    int argc = __argc;
    wchar_t** argv = __wargv;
    main(argc, argv);

    std::wcout.rdbuf(oldCoutBuf);
    std::wcerr.rdbuf(oldCerrBuf);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    switch (msg)
    {
        case WM_CREATE:
            g_hEdit = CreateWindowExW(WS_EX_CLIENTEDGE, L"EDIT", NULL,
                WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_HSCROLL |
                ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_AUTOHSCROLL | ES_READONLY,
                0, 0, 0, 0,
                hwnd, NULL, GetModuleHandle(NULL), NULL);

            g_hFont = CreateFontW(16, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE,
                                  DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS,
                                  DEFAULT_QUALITY, FIXED_PITCH | FF_MODERN, L"Fixedsys");
            SendMessageW(g_hEdit, WM_SETFONT, (WPARAM)g_hFont, TRUE);

            g_hThread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)GuiRedirectThread, NULL, 0, NULL);
            break;

        case WM_CTLCOLOREDIT:
        case WM_CTLCOLORSTATIC:
        {
            HDC hdc = (HDC)wParam;
            SetTextColor(hdc, RGB(255, 255, 255));
            SetBkColor(hdc, RGB(0, 0, 0));
            return (INT_PTR)GetStockObject(BLACK_BRUSH);
        }

        case WM_SIZE:
            if (g_hEdit)
                MoveWindow(g_hEdit, 0, 0, LOWORD(lParam), HIWORD(lParam), TRUE);
            break;

        case WM_DESTROY:
            if (g_hFont)
                DeleteObject(g_hFont);
            PostQuitMessage(0);
            break;

        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return 0;
}

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int nCmdShow)
{
    const wchar_t CLASS_NAME[] = L"EasyMidiRouterWindow";

    WNDCLASS wc = {};
    wc.lpfnWndProc   = WndProc;
    wc.hInstance     = hInstance;
    wc.lpszClassName = CLASS_NAME;
    wc.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
    wc.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));

    RegisterClass(&wc);

    HWND hwnd = CreateWindowEx(
        0, CLASS_NAME, L"EasyMidiRouter",
        WS_OVERLAPPEDWINDOW,
        CW_USEDEFAULT, CW_USEDEFAULT, 700, 500,
        NULL, NULL, hInstance, NULL
    );

    if (!hwnd)
        return 0;

    ShowWindow(hwnd, nCmdShow);
    UpdateWindow(hwnd);

    MSG msg = {};
    while (GetMessage(&msg, NULL, 0, 0)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return 0;
}
