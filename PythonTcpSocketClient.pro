#-------------------------------------------------
#
# Project created by QtCreator 2017-01-14T15:37:03
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets network

TARGET = RubyTcpSocketClient
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which as been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


CONFIG+=c++11
SOURCES += main.cpp\
        mainwindow.cpp \
    clientsocket.cpp

HEADERS  += mainwindow.h \
    clientsocket.h

INCLUDEPATH += /usr/local/include/opencv \
                /usr/local/include

LIBS +=/usr/local/lib/libopencv_calib3d.so \
/usr/local/lib/libopencv_contrib.so \
/usr/local/lib/libopencv_core.so \
/usr/local/lib/libopencv_features2d.so \
/usr/local/lib/libopencv_flann.so \
/usr/local/lib/libopencv_gpu.so \
/usr/local/lib/libopencv_highgui.so \
/usr/local/lib/libopencv_imgproc.so \
/usr/local/lib/libopencv_legacy.so \
/usr/local/lib/libopencv_ml.so \
/usr/local/lib/libopencv_nonfree.so \
/usr/local/lib/libopencv_objdetect.so \
/usr/local/lib/libopencv_ocl.so \
/usr/local/lib/libopencv_photo.so \
/usr/local/lib/libopencv_stitching.so \
/usr/local/lib/libopencv_superres.so \
/usr/local/lib/libopencv_ts.a \
/usr/local/lib/libopencv_video.so \
/usr/local/lib/libopencv_videostab.so

FORMS    += mainwindow.ui
