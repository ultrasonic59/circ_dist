QT       += core gui widgets

CONFIG   += c++17
CONFIG   += debug_and_release

TARGET   = CircleDistanceApp
TEMPLATE = app

SOURCES += \
    main.cpp \
    mainwindow.cpp \
    circledetector.cpp

HEADERS += \
    mainwindow.h \
    circledetector.h

# ========== OpenCV ==========
# Укажите путь к вашей установке OpenCV
# Пример для OpenCV 4.5.5, собранной под VS2019 x64

OPENCV_DIR = C:/opencv/build

INCLUDEPATH += $${OPENCV_DIR}/include

CONFIG(debug, debug|release) {
    # Debug
    LIBS += -L$${OPENCV_DIR}/x64/vc15/lib \
        -lopencv_world455d
} else {
    # Release
    LIBS += -L$${OPENCV_DIR}/x64/vc15/lib \
        -lopencv_world455
}
