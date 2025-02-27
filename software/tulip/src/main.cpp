/**
 *
 * This file is part of Tulip (http://tulip.labri.fr)
 *
 * Authors: David Auber and the Tulip development Team
 * from LaBRI, University of Bordeaux
 *
 * Tulip is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation, either version 3
 * of the License, or (at your option) any later version.
 *
 * Tulip is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 */

#include <QLocale>
#include <QProcess>
#include <QDir>

#include <QApplication>
#include <QMessageBox>
#include <QStandardPaths>
#include <QTcpSocket>

#include <tulip/PythonVersionChecker.h>
#include <tulip/TulipRelease.h>
#include <tulip/TulipSettings.h>
#include <tulip/TlpQtTools.h>

#include <CrashHandling.h>

#include "TulipMainWindow.h"
#include "TulipSplashScreen.h"
#include "PluginsCenter.h"
#include "FormPost.h"
#include <tulip/SystemDefinition.h>

#if defined(__APPLE__)
#include <sys/types.h>
#include <signal.h>
#include <tulip/PythonVersionChecker.h>
#endif

#ifdef WIN32
#include <windows.h>
#endif

#ifdef interface
#undef interface
#endif

void sendUsageStatistics() {
  QNetworkAccessManager *mgr = new QNetworkAccessManager;
  QObject::connect(mgr, SIGNAL(finished(QNetworkReply *)), mgr, SLOT(deleteLater()));
  mgr->get(QNetworkRequest(QUrl(QString("http://tulip.labri.fr/TulipStats/tulip_stats.php?tulip=") +
                                TULIP_VERSION + "&os=" + OS_PLATFORM)));
}

bool sendAgentMessage(int port, const QString &message) {
  bool result = true;

  QTcpSocket sck;
  sck.connectToHost(QHostAddress::LocalHost, port);
  sck.waitForConnected(1000);

  if (sck.state() == QAbstractSocket::ConnectedState) {
    sck.write(message.toUtf8());
    sck.flush();
  } else {
    result = false;
  }

  sck.close();
  return result;
}

void checkTulipRunning(const QString &perspName, const QString &fileToOpen, bool showAgent) {
  QFile lockFile(QDir(QStandardPaths::standardLocations(QStandardPaths::TempLocation).at(0))
                     .filePath("tulip.lck"));

  if (lockFile.exists() && lockFile.open(QIODevice::ReadOnly)) {
    QString agentPort = lockFile.readAll();
    bool ok;
    int n_agentPort = agentPort.toInt(&ok);

    if (ok && sendAgentMessage(n_agentPort, "HELLO\tHELLO")) {

      if (showAgent) {
        sendAgentMessage(n_agentPort, "SHOW_AGENT\tPROJECTS");
      }

      // if a file was passed as argument, forward it to the running instance
      if (!fileToOpen.isEmpty()) { // open the file passed as argument
        if (!perspName.isEmpty()) {
          sendAgentMessage(n_agentPort, "OPEN_PROJECT_WITH\t" + perspName + "\t" + fileToOpen);
        } else {
          sendAgentMessage(n_agentPort, "OPEN_PROJECT\t" + fileToOpen);
        }
      } else if (!perspName.isEmpty()) { // open the perspective passed as argument
        sendAgentMessage(n_agentPort, "CREATE_PERSPECTIVE\t" + perspName);
      }

      exit(0);
    }
  }

  lockFile.close();
  lockFile.remove();
}

#ifdef TULIP_BUILD_PYTHON_COMPONENTS
static void checkPython(TulipMainWindow *tmw) {
#if defined(__APPLE__)
  // no need to check when in mac bundle
  auto appDir = QApplication::applicationDirPath();
  if (appDir.contains(".app/Contents/"))
    return;
#endif
  if (!tlp::PythonVersionChecker::isPythonVersionMatching()) {

    QStringList installedPythons = tlp::PythonVersionChecker::installedVersions();

    QString requiredPython = "Python " + tlp::PythonVersionChecker::compiledVersion();
    requiredPython += " (64 bit)";

    QString errorMessage;

    errorMessage = requiredPython + " installation path cannot be found on your system.\n";
    if (installedPythons.size() > 0) {
      errorMessage += "Detected version(s): ";
      for (int i = 0; i < installedPythons.size(); ++i) {
        errorMessage += installedPythons.at(i);

        if (i < installedPythons.size() - 1) {
          errorMessage += ", ";
        }
      }

      errorMessage += ".";
    }
    QMessageBox::warning(tmw, requiredPython + " not found", errorMessage);

    tmw->showErrorMessage("Python", errorMessage);
  }
}
#endif

int main(int argc, char **argv) {
  CrashHandling::installCrashHandler();

  // Enables resource sharing between the OpenGL contexts
  QCoreApplication::setAttribute(Qt::AA_ShareOpenGLContexts, true);
  // Enables high-DPI scaling on X11 or Windows platforms
  // Enabled on MacOSX with NSHighResolutionCapable key in Info.plist file
  QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling, true);

  QApplication tulip_agent(argc, argv);
  QString name("Tulip ");

  // show patch number only if needed
  if (TULIP_INT_VERSION % 10)
    name += TULIP_VERSION;
  else
    name += TULIP_MM_VERSION;

  // the applicationName below is used to identify the location
  // of downloaded plugins, so it must be the same as in
  // tulip_perspective/main.cpp
  tulip_agent.setApplicationName(name);

  // Parse arguments
  QRegExp perspectiveRegexp("^\\-\\-perspective=(.*)");
  QString perspName;
  QString fileToOpen;
  bool showAgent = true;

  for (int i = 1; i < QApplication::arguments().size(); ++i) {
    QString s = QApplication::arguments()[i];

    if (perspectiveRegexp.exactMatch(s)) {
      perspName = perspectiveRegexp.cap(1);
    } else if (s == "--no-show-agent") {
      showAgent = false;
    } else {
      fileToOpen = s;
    }
  }

  showAgent = showAgent || fileToOpen.isEmpty();

#if defined(__APPLE__)
  auto appDir = QApplication::applicationDirPath();
  if (appDir.contains(".app/Contents/")) {
    // customize env when in macOS bundle
    auto path = appDir;
    // ensure to find all libs needed by tulip_perspective
    qputenv("DYLD_FRAMEWORK_PATH", path.append("/Frameworks").toLocal8Bit());
    qputenv("DYLD_FALLBACK_LIBRARY_PATH", path.toLocal8Bit());
    qputenv("DYLD_LIBRARY_PATH", path.toLocal8Bit());
    path = appDir;
    qputenv("QT_PLUGIN_PATH", path.append("/PlugIns").toLocal8Bit());
    qputenv("QT_QPA_PLATFORM_PLUGIN_PATH=", path.append("/platforms").toLocal8Bit());
    auto pyv = tlp::PythonVersionChecker::compiledVersion();
    if (!pyv.isEmpty()) {
      // ensure pip external modules can be installed and used through the gui
      path = appDir;
      qputenv("DYLD_LIBRARY_PATH", path.append("/Frameworks/Python.framework/Versions/")
                                       .append(pyv)
                                       .append("/lib:")
                                       .append(qgetenv("DYLD_LIBRARY_PATH"))
                                       .toLocal8Bit());
      path = appDir;
      qputenv("PATH", path.append("/Frameworks/Python.framework/Versions/")
                          .append(pyv)
                          .append("/bin:")
                          .append(qgetenv("PATH"))
                          .toLocal8Bit());
    }
  }
#endif

  checkTulipRunning(perspName, fileToOpen, showAgent);
  sendUsageStatistics();

  TulipSplashScreen splashScreen;
  tlp::initTulipSoftware(&splashScreen, true);

#ifdef _MSC_VER
  // Add path to Tulip pdb files generated by Visual Studio (for configurations Debug and
  // RelWithDebInfo)
  // It allows to get a detailed stack trace when Tulip crashes.
  CrashHandling::setExtraSymbolsSearchPaths(tlp::TulipShareDir + "pdb");
#endif

  // Main window
  TulipMainWindow *mainWindow = TulipMainWindow::instance();
  mainWindow->pluginsCenter()->reportPluginErrors(splashScreen.errors());

  mainWindow->show();
  splashScreen.finish(mainWindow);
#ifdef TULIP_BUILD_PYTHON_COMPONENTS
  checkPython(mainWindow);
#endif

  // Treat arguments
  if (!fileToOpen.isEmpty()) { // open the file passed as argument
    if (!perspName.isEmpty())
      mainWindow->openProjectWith(fileToOpen, perspName);
    else
      mainWindow->openProject(fileToOpen);
  } else if (!perspName.isEmpty()) { // open the perspective passed as argument
    mainWindow->createPerspective(perspName);
  }

  int result = tulip_agent.exec();

  QFile f(QDir(QStandardPaths::standardLocations(QStandardPaths::TempLocation).at(0))
              .filePath("tulip.lck"));
  f.remove();

  return result;
}
