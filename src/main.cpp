#include <QApplication>

#include <QCoreApplication>
#include <QTranslator>
#include <QDir>
#include <cstring>
#include <QDateTime>

#include "mainwindow.h"
#include "settings.h"
#include "banpair.h"
#include "server.h"
#include "audio.h"

int main(int argc, char *argv[])
{
	QApplication a(argc, argv);

#ifdef Q_OS_MAC
#ifdef QT_NO_DEBUG

	QDir::setCurrent(qApp->applicationDirPath());

#endif
#endif

	// initialize random seed for later use
	qsrand(QTime(0,0,0).secsTo(QTime::currentTime()));

	QTranslator qt_translator, translator;
	qt_translator.load("qt_zh_CN.qm");
	translator.load("zh_CN.qm");

	qApp->installTranslator(&qt_translator);
	qApp->installTranslator(&translator);

	Bang = new Engine;
	Config.init();
	BanPair::loadBanPairs();

	if(qApp->arguments().contains("-server")){
		Server *server = new Server(qApp);

		/*@to-do: the room thread & the room driver thread may not stop before the main thread runs into Room::~Room()
or RoomDriver()::~RoomDriver(), which will block the main thread itself and room thread(living in the main thread)
can't stop. That's a deadlock. That's why main window has to wait(not blocked) for a ready_to_close() signal from
Server to call close() again.*/
		QObject::connect(qApp, SIGNAL(aboutToQuit()), server, SIGNAL(about_to_stop()));
		QObject::connect(server, SIGNAL(ready_to_close()), Bang, SLOT(deleteLater()));

		printf("Server is starting on port %u\n", Config.ServerPort);
		if(server->listen())
			printf("Starting successfully\n");
		else
			printf("Starting failed!\n");

		return qApp->exec();
	}

	QFile file("bang.qss");
	if(file.open(QIODevice::ReadOnly)){
		QTextStream stream(&file);
		qApp->setStyleSheet(stream.readAll());
	}

#ifdef AUDIO_SUPPORT
	Audio::init();
#endif

	MainWindow main_window;
	main_window.show();

	Bang->setParent(&main_window);

	foreach(QString arg, qApp->arguments()){
		if(arg.startsWith("-connect:")){
			arg.remove("-connect:");
			Config.HostAddress = arg;
			Config.setValue("HostAddress", arg);

			main_window.startConnection();

			break;
		}
	}

	return qApp->exec();
}
