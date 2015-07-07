#include <qapplication.h>
#include "window.h"
extern "C" {
    void start_viewer(int argc, char *argv[]);
}

int main(int argc, char *argv[])
{
    start_viewer(argc, argv);
    QApplication a(argc, argv);
    window w;
    a.setMainWidget(&w);
    w.show();
    return a.exec();
}

