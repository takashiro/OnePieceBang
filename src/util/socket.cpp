#include "socket.h"
#include "settings.h"

#include <QTcpSocket>
#include <QRegExp>
#include <QStringList>
#include <QUdpSocket>
#include <QDebug>

ServerSocket::ServerSocket()
{
	server = new QTcpServer(this);
	daemon = NULL;

	connect(server, SIGNAL(newConnection()), this, SLOT(processNewConnection()));
}

bool ServerSocket::listen(){
	return server->listen(QHostAddress::Any, Config.ServerPort);
}

void ServerSocket::daemonize(){
	daemon = new QUdpSocket(this);
	daemon->bind(Config.ServerPort, QUdpSocket::ShareAddress);

	connect(daemon, SIGNAL(readyRead()), this, SLOT(processNewDatagram()));
}

void ServerSocket::processNewDatagram(){
	while(daemon->hasPendingDatagrams()){
		QHostAddress from;
		char ask_str[256];

		daemon->readDatagram(ask_str, sizeof(ask_str), &from);

		QByteArray data = Config.ServerName.toUtf8();
		daemon->writeDatagram(data, from, Config.DetectorPort);
	}
}

void ServerSocket::processNewConnection(){
	QTcpSocket *socket = server->nextPendingConnection();
	ClientSocket *connection = new ClientSocket(socket);
	emit new_connection(connection);
}

// ---------------------------------

ClientSocket::ClientSocket()
	:socket(new QTcpSocket(this))
{
	init();
}

ClientSocket::ClientSocket(QTcpSocket *socket)
	:socket(socket)
{
	socket->setParent(this);
	init();
}

void ClientSocket::init(){
	connect(socket, SIGNAL(disconnected()), this, SIGNAL(disconnected()));
	connect(socket, SIGNAL(error(QAbstractSocket::SocketError)), this, SLOT(raiseError(QAbstractSocket::SocketError)));
	connect(socket, SIGNAL(readyRead()), this, SIGNAL(readyRead()));
	connect(socket, SIGNAL(connected()), this, SIGNAL(connected()));
}

void ClientSocket::connectToHost(){
	QString address = "127.0.0.1";
	ushort port = 9527u;

	if(Config.HostAddress.contains(QChar(':'))){
		QStringList texts = Config.HostAddress.split(QChar(':'));
		address = texts.value(0);
		port = texts.value(1).toUShort();
	}else{
		address = Config.HostAddress;
		if(address == "127.0.0.1")
			port = Config.value("ServerPort","9527").toString().toUShort();
	}

	socket->connectToHost(address, port);
}

void ClientSocket::getMessage(){
	while(socket->canReadLine()){
		QByteArray msg = socket->readLine();
		if(msg.endsWith("\n")){
			msg.chop(1);
		}

#ifndef QT_NO_DEBUG
		if(!this->parent() || !this->parent()->inherits("Client")){
			qDebug() << this << this->parent() << msg;
		}
#endif
		emit message_got(msg);
	}
}

void ClientSocket::disconnectFromHost(){
	socket->disconnectFromHost();
}

void ClientSocket::listen(){
	connect(socket, SIGNAL(readyRead()), this, SLOT(getMessage()));
}

void ClientSocket::send(const QByteArray &raw_message){
	socket->write(raw_message);
	socket->write("\n");
}

void ClientSocket::send(const BP::Packet &packet){
	socket->write(packet.toUtf8());
	socket->write("\n");
}

bool ClientSocket::isConnected() const{
	return socket->state() == QTcpSocket::ConnectedState;
}

QString ClientSocket::peerName() const{
	QString peer_name = socket->peerName();
	if(peer_name.isEmpty())
		peer_name = QString("%1:%2").arg(socket->peerAddress().toString()).arg(socket->peerPort());

	return peer_name;
}

QString ClientSocket::peerAddress() const{
	return socket->peerAddress().toString();
}

QByteArray ClientSocket::readLine(qint64 len){
	return socket->readLine(len);
}

bool ClientSocket::canReadLine() const{
	return socket->canReadLine();
}

void ClientSocket::raiseError(QAbstractSocket::SocketError socket_error){
	// translate error message
	QString reason;
	switch(socket_error){
	case QAbstractSocket::ConnectionRefusedError:
		reason = tr("Connection was refused or timeout"); break;
	case QAbstractSocket::RemoteHostClosedError:
		reason = tr("Remote host close this connection"); break;
	case QAbstractSocket::HostNotFoundError:
		reason = tr("Host not found"); break;
	case QAbstractSocket::SocketAccessError:
		reason = tr("Socket access error"); break;
	case QAbstractSocket::NetworkError:
		return; // this error is ignore ...
	default: reason = tr("Unknow error"); break;
	}

	emit error_message(tr("Connection failed, error code = %1\n reason:\n %2").arg(socket_error).arg(reason));
}
