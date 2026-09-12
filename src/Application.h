// SPDX-License-Identifier: BSD-3-Clause
// SPDX-FileCopyrightText: The Monero Project

#ifndef BESTWALLET_APPLICATION_H
#define BESTWALLET_APPLICATION_H

#include <QApplication>
#include <QtNetwork/qlocalserver.h>

class QLockFile;

class Application : public QApplication {
    Q_OBJECT

public:
    Application(int& argc, char** argv);
    ~Application() override;

    bool isAlreadyRunning() const;

signals:
    void anotherInstanceStarted();

private slots:
    void processIncomingConnection();

private:
    // Registers the bundled typefaces before any widget is built.
    void loadFonts();

    bool m_alreadyRunning;
    QLockFile* m_lockFile;
    QLocalServer m_lockServer;
    QString m_socketName;
};


#endif //BESTWALLET_APPLICATION_H
