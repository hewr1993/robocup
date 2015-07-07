#ifndef WINDOW_H
#define WINDOW_H

#include <qwidget.h>

#define MAX_MJPEG_SIZE 200000
#define MAX_NET_WIDTH	320
#define MAX_NET_HEIGHT	240
class QImage;

class window: public QWidget {

    Q_OBJECT
private:
    QImage *pRGB[2];

private slots:
    void get_data1();
    void get_data2();

public:
    window();
    ~window();

protected:
    void paintEvent(QPaintEvent *);
    void mousePressEvent(QMouseEvent *e);
};

#endif

