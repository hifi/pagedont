#include <QApplication>
#include <QtNetwork>
#include "BinaryStream.h"

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

int main(int argc, char** argv)
{
    QApplication app(argc, argv);


    return app.exec();
}
