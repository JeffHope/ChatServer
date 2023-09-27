#include "server.h"
#include <QThread>
#include <QApplication>

int main(int argc, char *argv[]) {
    QApplication a(argc, argv);
    Server w;
    w.listen(QHostAddress::Any, 65000);
    return a.exec();
}
