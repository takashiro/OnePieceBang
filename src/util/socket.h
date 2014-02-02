#ifndef SOCKET_H
#define SOCKET_H

#include <QObject>
#include <QTcpSocket>
#include <QTcpServer>
#include "protocol.h"

class QUdpSocket;
class ClientSocket;

class ServerSocket: public QObject{
	Q_OBJECT

public:
	ServerSocket();

	bool listen();
	void daemonize();

private slots:
	void processNewConnection();
	void processNewDatagram();

private:
	QTcpServer *server;
	QUdpSocket *daemon;

signals:
	void new_connection(ClientSocket *connection);
};


class ClientSocket: public QObject{
	Q_OBJECT

public:
	ClientSocket();
	ClientSocket(QTcpSocket *socket);

	void connectToHost();
	void disconnectFromHost();
	bool isConnected() const;
	QString peerName() const;
	QString peerAddress() const;
	QByteArray readLine(qint64 len = 0);
	bool canReadLine() const;

public slots:
	void send(const QByteArray &raw_message);
	void send(const BP::Packet &packet);
	void listen();

private slots:
	void getMessage();
	void raiseError(QAbstractSocket::SocketError socket_error);

private:
	QTcpSocket * const socket;

	void init();

signals:
	void message_got(QByteArray msg);
	void error_message(const QString &msg);
	void disconnected();
	void connected();
	void readyRead();
};

#endif // SOCKET_H
