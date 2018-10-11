#include <QApplication>
#include <QDesktopWidget>
#include <QPixmap>
#include <QSplashScreen>
#include <QStyle>
#include <QTextStream>
#include <QFileInfo>
#include <cameraselectdialog.h>

#include <unistd.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <signal.h>

#include "lvmainwindow.h"

#ifndef HOST
#define HOST "unknown location"
#endif

#ifndef UNAME
#define UNAME "unknown person"
#endif

#define handle_error(msg) \
    do { perror(msg); exit(EXIT_FAILURE); } while(0)

int main(int argc, char* argv[])
{
    int sfd;
    struct sockaddr_un lv_addr;
    QApplication a(argc, argv);

    sfd = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sfd == -1) {
        handle_error("socket");
        return EXIT_FAILURE;
    }
    memset(&lv_addr, 0, sizeof(struct sockaddr_un));
    lv_addr.sun_family = AF_UNIX;
    strncpy(lv_addr.sun_path, "LiveViewOpenSource",
            sizeof(lv_addr.sun_path) - 1);
    if (bind(sfd, reinterpret_cast<struct sockaddr*>(&lv_addr),
             sizeof(struct sockaddr_un)) == -1) {
        auto reply = QMessageBox::question(nullptr, "LiveView Cannot Start",
                            "Only one instance of LiveView should be run at a time. Multiple instances can cause errors. Would you like to continue anyways?",
                                           QMessageBox::Yes | QMessageBox::Cancel);
        if (reply == QMessageBox::Cancel) {
            handle_error("bind");
        } else {
            /* lockfile.open(QIODevice::ReadWrite | QIODevice::Text);
            QString line = lockfile_stream.readLine();
            // Do some error checking here so it doesn't just die if the process isn't owned by the user
            auto pid = line.toInt();
            if (pid == 0 || kill(pid, SIGKILL) == -1) {
                QMessageBox::information(nullptr, "Cannot kill LiveView", "The currently open LiveView process cannot be killed. To resolve this, manually end the other LiveView process or restart your system. LiveView will now Close.", QMessageBox::Button::Abort);
                return -1;
            } */
            unlink("LiveViewOpenSource");
            bind(sfd, reinterpret_cast<struct sockaddr*>(&lv_addr), sizeof(struct sockaddr_un));
        }
    }
    if (listen(sfd, 50) == -1) {
        handle_error("listen");
    }

    /* QFile lockfile{"/tmp/.LiveView-lock"};
    QFileInfo lockfile_info{"/tmp/.LiveView-lock"};
    QTextStream lockfile_stream{&lockfile};
    int pid_status;
    if (lockfile_info.exists() || !lockfile.open(QIODevice::ReadWrite | QIODevice::Text)) {
        lockfile.open(QIODevice::ReadWrite | QIODevice::Text);
        QString line = lockfile_stream.readLine();
        auto pid = line.toInt();
        if (waitpid(pid, &pid_status, WNOHANG) != 0) {

        }
        auto reply = QMessageBox::question(nullptr, "LiveView Cannot Start",
                            "Only one instance of LiveView may be run at a time. Would you like the other LiveView instance to be stopped?",
                                           QMessageBox::Yes|QMessageBox::Cancel);
        if (reply == QMessageBox::Cancel) {
            return EXIT_FAILURE;
        } else {
            lockfile.open(QIODevice::ReadWrite | QIODevice::Text);
            QString line = lockfile_stream.readLine();
            // Do some error checking here so it doesn't just die if the process isn't owned by the user
            auto pid = line.toInt();
            if (pid == 0 || kill(pid, SIGKILL) == -1) {
                QMessageBox::information(nullptr, "Cannot kill LiveView", "The currently open LiveView process cannot be killed. To resolve this, manually end the other LiveView process or restart your system. LiveView will now Close.", QMessageBox::Button::Abort);
                return -1;
            }
        }
    } */

    /* int my_pid = getpid();
    qDebug() << "PID:" << my_pid;
    lockfile.close();
    lockfile.open(QIODevice::WriteOnly | QIODevice::Text | QIODevice::Truncate);
    lockfile_stream << my_pid;
    lockfile.close(); */
    QSettings settings(QStandardPaths::writableLocation(
                           QStandardPaths::ConfigLocation)
                       + "/lvconfig.ini", QSettings::IniFormat);

    if (settings.value(QString("dark"), USE_DARK_STYLE).toBool()) {
        QFile f(":qdarkstyle/style.qss");

        if (!f.exists()) {
            printf("Unable to set stylesheet, file not found\n");
        } else {
            f.open(QFile::ReadOnly | QFile::Text);
            QTextStream ts(&f);
            qApp->setStyleSheet(ts.readAll());
        }
    }

    CameraSelectDialog csd(&settings);
    if (settings.value(QString("show_cam_dialog"), true).toBool()) {
        int retval = csd.exec();
        if (!retval) {
            unlink("LiveViewOpenSource");
            // lockfile.remove();
            return -1;
        }
    }

    QPixmap logo_pixmap(":images/aviris-logo-transparent.png");
    QSplashScreen splash(logo_pixmap);
    splash.show();
    splash.showMessage(QObject::tr("Loading LiveView... Compiled on " __DATE__ ", " __TIME__ " PDT by " UNAME "@" HOST),
                       Qt::AlignCenter | Qt::AlignBottom, Qt::gray);

    qDebug() << "This version (" << GIT_CURRENT_SHA1_SHORT << ") of LiveView was compiled on"
             << __DATE__ << "at" << __TIME__ << "using gcc" << __GNUC__;
    qDebug() << "The compilation was performed by" << UNAME << "@" << HOST;

    LVMainWindow w(&settings);
    w.setGeometry(QStyle::alignedRect(
                      Qt::LeftToRight,
                      Qt::AlignCenter,
                      w.size(),
                      a.desktop()->availableGeometry()));
    w.show();
    splash.finish(&w);

    auto ret_val = a.exec();
    unlink("LiveViewOpenSource");
    // lockfile.remove();

    return ret_val;
}
