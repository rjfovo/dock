/*
 * Copyright (C) 2021 CutefishOS Team.
 *
 * Author:     rekols <revenmartin@gmail.com>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <QApplication>
#include <QQmlApplicationEngine>
#include <QQuickView>
#include <QQuickStyle>
#include <QTranslator>
#include <QLocale>
#include <QDBusConnection>
#include <QThread>
#include <QIcon>
#include <QDir>
#include <QDebug>

#include "applicationmodel.h"
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 显式指定 Qt Quick Controls 主题为 fish-style：
    // 会话设置了 QT_STYLE_OVERRIDE=cutefish，QQuickStyle 解析时优先用它（而不是
    // QT_QUICK_CONTROLS_STYLE），导致回退到 Basic 的"默认 demo 样式"。
    QQuickStyle::setStyle(QStringLiteral("fish-style"));

    // 设置图标主题
    // 在Qt6中，需要确保图标主题可用
    // 首先设置搜索路径
    QStringList iconThemePaths;
    iconThemePaths << "/usr/share/icons";
    iconThemePaths << QDir::homePath() + "/.local/share/icons";
    iconThemePaths << "/usr/local/share/icons";
    QIcon::setThemeSearchPaths(iconThemePaths);
    
    // 尝试按优先级设置图标主题
    QStringList preferredThemes = {"cutefish", "Crule", "Crule-dark", "breeze", "Adwaita", "hicolor"};
    QString themeSet = "hicolor"; // 默认回退
    
    for (const QString &theme : preferredThemes) {
        QString themePath = QString("/usr/share/icons/%1").arg(theme);
        if (QDir(themePath).exists()) {
            themeSet = theme;
            break;
        }
    }
    
    QIcon::setThemeName(themeSet);
    qDebug() << "Dock: Icon theme set to:" << QIcon::themeName() << "from search paths:" << QIcon::themeSearchPaths();
    
    // 确保QIcon图像提供者可用于QML
    // 这是image://icontheme/ URL正常工作所必需的
    if (QIcon::themeName().isEmpty()) {
        qWarning() << "Dock: No icon theme set! image://icontheme/ URLs will not work.";
    }

    // Try multiple times to register DBus service (in case DBus is not ready yet)
    int retryCount = 0;
    const int maxRetries = 10;
    bool serviceRegistered = false;
    
    while (retryCount < maxRetries && !serviceRegistered) {
        serviceRegistered = QDBusConnection::sessionBus().registerService("com.cutefish.Dock");
        if (!serviceRegistered) {
            retryCount++;
            QThread::msleep(100); // Wait 100ms before retrying
        }
    }
    
    if (!serviceRegistered) {
        qWarning() << "Failed to register DBus service after" << maxRetries << "retries";
        return -1;
    }

    qmlRegisterType<DockSettings>("Cutefish.Dock", 1, 0, "DockSettings");

    QString qmFilePath = QString("%1/%2.qm").arg("/usr/share/cutefish-dock/translations/").arg(QLocale::system().name());
    if (QFile::exists(qmFilePath)) {
        QTranslator *translator = new QTranslator(QApplication::instance());
        if (translator->load(qmFilePath)) {
            QGuiApplication::installTranslator(translator);
        } else {
            translator->deleteLater();
        }
    }

    MainWindow w;

    if (!QDBusConnection::sessionBus().registerObject("/Dock", &w)) {
        qWarning() << "Failed to register DBus object";
        return -1;
    }

    return app.exec();
}
