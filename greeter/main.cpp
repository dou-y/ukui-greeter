/* main.cpp
 * Copyright (C) 2018 Tianjin KYLIN Information Technology Co., Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2, or (at your option)
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301, USA.
**/
#include "greeterwindow.h"
#include <stdio.h>
#include <string>
#include <QApplication>
#include <QTextCodec>
#include <QResource>
#include <QTranslator>
#include <QLocale>
#include <QDesktopWidget>
#include <QRect>
#include <QDateTime>
#include <QDebug>
#include <QStandardPaths>
#include <QX11Info>
#include <QThread>
#include <QProcess>
#include <X11/Xlib.h>
#include <X11/cursorfont.h>
#include <X11/extensions/Xrandr.h>
#include "globalv.h"
#include "mainwindow.h"

QString configFile;
QLocale::Language language;

void waitMonitorsReady()
{
    int n;
    Display *dpy = XOpenDisplay(NULL);
    int screen = DefaultScreen(dpy);
    Window root = RootWindow(dpy, screen);
    while(true){
        XRRGetMonitors(dpy, root, false, &n);
        if(n == -1)
            qWarning() << "get monitors failed";
        else if(n > 0){
            qDebug() << "found " << n << " monitors";
            return;
        }
        //启动xrandr，打开视频输出， 自动设置最佳分辨率
        QProcess enableMonitors;
        enableMonitors.start("xrandr --auto");
        enableMonitors.waitForFinished(-1);
    }
}

void outputMessage(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    Q_UNUSED(context)
    QDateTime dateTime = QDateTime::currentDateTime();
    QByteArray time = QString("[%1] ").arg(dateTime.toString("MM-dd hh:mm:ss.zzz")).toLocal8Bit();
    QByteArray localMsg = msg.toLocal8Bit();
    switch(type) {
    case QtDebugMsg:
        fprintf(stderr, "%s Debug: %s:%u: %s\n", time.constData(), context.file, context.line, localMsg.constData());
        break;
#ifdef VERSION_ENOUGH
    case QtInfoMsg:
        fprintf(stderr, "%s Info: %s:%u: %s\n", time.constData(), context.file, context.line, localMsg.constData());
        break;
#endif
    case QtWarningMsg:
        fprintf(stderr, "%s Warnning: %s:%u: %s\n", time.constData(), context.file, context.line, localMsg.constData());
        break;
    case QtCriticalMsg:
        fprintf(stderr, "%s Critical: %s:%u: %s\n", time.constData(), context.file, context.line, localMsg.constData());
        break;
    case QtFatalMsg:
        fprintf(stderr, "%s Fatal: %s:%u: %s\n", time.constData(), context.file, context.line, localMsg.constData());
        abort();
    }

}

int main(int argc, char *argv[])
{
    qInstallMessageHandler(outputMessage);
    QApplication a(argc, argv);

    QResource::registerResource("image.qrc");

    QTextCodec *codec = QTextCodec::codecForName("UTF-8");
    QTextCodec::setCodecForLocale(codec);

    language = QLocale::system().language();
    qDebug() << "current locale language: " << QLocale::languageToString(language);

    //加载翻译文件
    QTranslator translator;
    if(language == QLocale::Chinese) {
        translator.load(QM_DIR + "zh_CN.qm");
    }
    a.installTranslator(&translator);

    //加载qss文件
    QFile qss(":/ukui-greeter.qss");
    qss.open(QFile::ReadOnly);
    a.setStyleSheet(qss.readAll());
    qss.close();

    //用于QSettings
    QApplication::setOrganizationName("Kylin");
    QApplication::setApplicationName("ukui-greeter");

    configFile = QStandardPaths::displayName(QStandardPaths::CacheLocation) + "/ukui-greeter.conf";
    qDebug() << "load configure file: "<< configFile;

    //设置鼠标指针样式
    XDefineCursor(QX11Info::display(), QX11Info::appRootWindow(), XCreateFontCursor(QX11Info::display(), XC_arrow));

    //等待显示器准备完毕
    waitMonitorsReady();

    MainWindow w;
    w.show();
    //在没有窗口管理器的情况下，需要激活窗口，行为类似于用鼠标点击窗口
    w.activateWindow();

    return a.exec();
}