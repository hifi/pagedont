#include <QApplication>
#include <QtNetwork>
#include "BinaryStream.h"
#include <windows.h>

bool sendMessage(const QByteArray& in, QByteArray& out)
{
    QLocalSocket socket;
    BinaryStream stream(&socket);

    socket.connectToServer("\\\\.\\pipe\\openssh-ssh-agent");
    if (!socket.waitForConnected(500)) {
        return false;
    }

    stream.writeString(in);
    stream.flush();

    if (!stream.readString(out)) {
        return false;
    }

    socket.close();

    return true;
}

#define AGENT_MAX_MSGLEN 8192
#define AGENT_COPYDATA_ID 0x804e50ba

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    if (uMsg == WM_COPYDATA) {
        COPYDATASTRUCT *data = (COPYDATASTRUCT *)lParam;
        if (data->dwData != AGENT_COPYDATA_ID) {
            MessageBoxA(NULL, "NOT IT", "NOT IT", 0);
        } else {
            HANDLE handle = OpenFileMappingA(FILE_MAP_WRITE, false, (char *)data->lpData);
            LPVOID ptr = MapViewOfFile(handle, FILE_MAP_WRITE, 0, 0, 0);

            if (!ptr) {
                MessageBoxA(NULL, "Mapping failed", "PageDon't", 0);
                return false;
            }

            quint32 requestLength = qFromBigEndian<quint32>(reinterpret_cast<quint32*>(ptr));
            void* requestData = reinterpret_cast<void*>(reinterpret_cast<char*>(ptr) + 4);

            QByteArray in;
            QByteArray out;

            in.resize(requestLength);
            memcpy(in.data(), requestData, requestLength);

            if (!sendMessage(in, out)) {
                MessageBoxA(NULL, "OpenSSH message failed", "PageDon't", 0);
            }

            *(quint32*)ptr = qToBigEndian<quint32>(out.length());
            memcpy(requestData, out.data(), out.length());
            return true;
        }
    }

    return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
{
    WNDCLASS wc;
    memset(&wc, 0, sizeof(wc));
    wc.lpfnWndProc = WindowProc;
    wc.hInstance = hInstance;
    wc.lpszClassName = "Pageant";

    RegisterClassA(&wc);
    HWND win = CreateWindowExA(0, "Pageant", "Pageant", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, 100, 100, NULL, NULL, hInstance, NULL);

    if (!win)
        return 0;

    ShowWindow(win, true);

    MSG msg;
    BOOL bRet;
    while (1)
    {
        bRet = GetMessage(&msg, NULL, 0, 0);
	    if (bRet > 0)
	    {
	        TranslateMessage(&msg);
            DispatchMessage(&msg);
	    } else {
            break;
        }
    }

    return msg.wParam;
}
