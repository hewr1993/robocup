TEMPLATE = app
INCLUDEPATH += ../include

HEADERS += window.h jpegdec.h
SOURCES += jpegdec.c net2_show.cpp window.cpp process.c
LIBS += -ljpeg ../librobot/librobot.a
