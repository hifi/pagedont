/*
 * Copyright (c) 2020 Toni Spets <toni.spets@iki.fi>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <windows.h>

#include <stdio.h>
#define OutputDebugStringA _debugprint
static void _debugprint(const char *msg)
{
    printf("Debug: %s\n", msg);
}

#define AGENT_MAX_MSGLEN 8192
#define AGENT_COPYDATA_ID 0x804e50ba

#define BSWAP32(x) ((((x) & 0x000000ff) << 24) | \
                    (((x) & 0x0000ff00) << 8)  | \
                    (((x) & 0x00ff0000) >> 8)  | \
                    (((x) & 0xff000000) >> 24));

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    LRESULT ret = FALSE;
    HANDLE hFile = NULL,
           hPipe = NULL;
    LPVOID lpView = NULL,
           lpData = NULL;
    DWORD dwLength, dwWritten, dwMode;

    if (uMsg != WM_COPYDATA)
        return DefWindowProc(hwnd, uMsg, wParam, lParam);

    COPYDATASTRUCT *data = (COPYDATASTRUCT *)lParam;

    if (data->dwData != AGENT_COPYDATA_ID || data->lpData == NULL)
        goto error;

    OutputDebugStringA("Processing Pageant message");

    if ((hFile = OpenFileMappingA(FILE_MAP_WRITE, FALSE, (char *)data->lpData)) == INVALID_HANDLE_VALUE)
        goto error;

    if ((lpView = MapViewOfFile(hFile, FILE_MAP_WRITE, 0, 0, 0)) == NULL)
        goto error;

    dwLength = BSWAP32(*(DWORD*)lpView);
    lpData = (BYTE*)lpView + 4;

    if (dwLength < 1 || dwLength > AGENT_MAX_MSGLEN - 4)
        goto error;

    OutputDebugStringA("Message validated, connecting to ssh-agent pipe");

    while (1) {
        if ((hPipe = CreateFile("\\\\.\\pipe\\openssh-ssh-agent", GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL)) != INVALID_HANDLE_VALUE)
            break;

        if (GetLastError() != ERROR_PIPE_BUSY)
            goto error;

        if (!WaitNamedPipe("\\\\.\\pipe\\openssh-ssh-agent", 5000))
            goto error;
    }

    OutputDebugStringA("Agent pipe opened, writing message");

    dwMode = PIPE_READMODE_MESSAGE;
    if (!SetNamedPipeHandleState(hPipe, &dwMode, NULL, NULL))
        goto error;

    if (!WriteFile(hPipe, lpData, dwLength, &dwWritten, NULL))
        goto error;

    if (dwWritten != dwLength)
        goto error;

    OutputDebugStringA("Message written, reading reaply");

    // FIXME: this could theoretically return less data than a complete message
    if (!ReadFile(hPipe, lpData, AGENT_MAX_MSGLEN - 4, &dwLength, NULL))
        goto error;

    if (dwLength < 1 || dwLength > AGENT_MAX_MSGLEN - 4)
        goto error;

    OutputDebugStringA("Message read, setting success");

    *(DWORD*)lpView = BSWAP32(dwLength);
    ret = TRUE;

error:
    OutputDebugStringA("Cleaning up");

    if (hPipe)
        CloseHandle(hPipe);

    if (lpView)
        UnmapViewOfFile(lpView);

    if (hFile)
        CloseHandle(hFile);

    return ret;
}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    WNDCLASS wc;
    MSG msg;
    HWND hWnd;

    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "Pageant";

    RegisterClassA(&wc);

    hWnd = CreateWindowExA(0, "Pageant", "Pageant", 0, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, HWND_MESSAGE, NULL, hInstance, NULL);
    if (!hWnd) {
        OutputDebugStringA("Failed to create window.");
        return 1;
    }

    ShowWindow(hWnd, TRUE);

    while (GetMessage(&msg, NULL, 0, 0) > 0)
    {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }

    return msg.wParam;
}
