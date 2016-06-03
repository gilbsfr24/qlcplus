/*
  Q Light Controller Plus
  huecontroller.cpp

  Copyright (c) Massimo Callegari

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

      http://www.apache.org/licenses/LICENSE-2.0.txt

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include "huecontroller.h"

#include <QMutexLocker>
#include <QByteArray>
#include <QDebug>
#include <QColor>

HUEController::HUEController(QString ipaddr, Type type, quint32 line, QObject *parent)
    : QObject(parent)
{
    m_ipAddr = QHostAddress(ipaddr);
    m_line = line;

    qDebug() << "[HUEController] type: " << type;
    m_packetizer.reset(new HUEPacketizer());
    m_packetSent = 0;
    m_packetReceived = 0;

    m_outputSocket = new QTcpSocket(this);
}

HUEController::~HUEController()
{
    qDebug() << Q_FUNC_INFO;
    //disconnect(m_outputSocket, SIGNAL(readyRead()), this, SLOT(processPendingPackets()));
    if (m_outputSocket->isOpen())
        m_outputSocket->close();
    QMapIterator<quint32, QByteArray *> it(m_dmxValuesMap);
    while(it.hasNext())
    {
        it.next();
        QByteArray *ba = it.value();
        delete ba;
    }
    m_dmxValuesMap.clear();
}

QHostAddress HUEController::getNetworkIP() const
{
    return m_ipAddr;
}

void HUEController::addUniverse(quint32 universe, HUEController::Type type)
{
    qDebug() << "[HUE] addUniverse - universe" << universe << ", type" << type;
    if (m_universeMap.contains(universe))
    {
        m_universeMap[universe].type |= (int)type;
    }
    else
    {
        UniverseInfo info;
        info.inputSocket = NULL;
        info.inputPort = 7700 + universe;
        if (m_ipAddr == QHostAddress::LocalHost)
        {
            info.feedbackAddress = QHostAddress::LocalHost;
            info.outputAddress = QHostAddress::LocalHost;
        }
        else
        {
            info.feedbackAddress = QHostAddress::Null;
            info.outputAddress = QHostAddress::Null;
            info.outputUser = "newdeveloper";
        }
        info.feedbackPort = 9000 + universe;
        info.outputPort = 80;
        info.type = type;
        m_universeMap[universe] = info;
        m_portsMap[info.inputPort] = universe;
    }

    QTcpSocket *socket = m_universeMap[universe].inputSocket;
    if (type == Input && socket == NULL)
    {
        quint16 inPort = m_universeMap[universe].inputPort;
        socket = new QTcpSocket(this);
        if (socket->bind(inPort, QTcpSocket::ShareAddress | QTcpSocket::ReuseAddressHint) == false)
        {
            qDebug() << "[HUEController] failed to bind socket to port" << inPort;
            delete m_universeMap[universe].inputSocket;
            m_universeMap[universe].inputSocket = NULL;
        }
        else
            connect(socket, SIGNAL(readyRead()),
                    this, SLOT(processPendingPackets()));
    }
}

void HUEController::removeUniverse(quint32 universe, HUEController::Type type)
{
    if (m_universeMap.contains(universe))
    {
        if (type == Input)
        {
            m_portsMap.take(m_universeMap[universe].inputPort);
            if (m_universeMap[universe].inputSocket != NULL)
            {
                disconnect(m_universeMap[universe].inputSocket, SIGNAL(readyRead()),
                           this, SLOT(processPendingPackets()));
                delete m_universeMap[universe].inputSocket;
            }
        }
        if (m_universeMap[universe].type == type)
            m_universeMap.take(universe);
        else
            m_universeMap[universe].type &= ~type;
    }
}

bool HUEController::setInputPort(quint32 universe, quint16 port)
{
    if (!m_universeMap.contains(universe))
        return false;

    QMutexLocker locker(&m_dataMutex);
    if (m_portsMap.contains(m_universeMap[universe].inputPort))
        m_portsMap.take(m_universeMap[universe].inputPort);
    m_universeMap[universe].inputPort = port;
    m_portsMap[port] = universe;

    QTcpSocket *socket = m_universeMap[universe].inputSocket;
    if (socket == NULL)
    {
        socket = new QTcpSocket(this);
        qDebug() << "[HUE] new input socket created on universe" << universe;
    }
    else
    {
        disconnect(socket, SIGNAL(readyRead()),
                this, SLOT(processPendingPackets()));
        qDebug() << "[HUE] socket disconnected from universe" << universe;
    }

    if (socket->bind(port, QTcpSocket::ShareAddress | QTcpSocket::ReuseAddressHint) == false)
    {
        qDebug() << "[HUEController] failed to bind socket to port" << port;
        return false;
    }

    qDebug() << "[HUE] socket connected for universe" << universe;
    connect(socket, SIGNAL(readyRead()),
            this, SLOT(processPendingPackets()));
    m_universeMap[universe].inputSocket = socket;

    return port == 7700 + universe;
}

bool HUEController::setFeedbackIPAddress(quint32 universe, QString address)
{
    if (m_universeMap.contains(universe) == false)
        return false;

    QMutexLocker locker(&m_dataMutex);
    m_universeMap[universe].feedbackAddress = QHostAddress(address);

    if (m_ipAddr == QHostAddress::LocalHost)
        return QHostAddress(address) == QHostAddress::LocalHost;
    return QHostAddress(address) == QHostAddress::Null;
}

bool HUEController::setFeedbackPort(quint32 universe, quint16 port)
{
    if (m_universeMap.contains(universe) == false)
        return false;

    QMutexLocker locker(&m_dataMutex);
    if (m_universeMap.contains(universe))
        m_universeMap[universe].feedbackPort = port;

    return port == 9000 + universe;
}

bool HUEController::setOutputIPAddress(quint32 universe, QString address)
{
    if (m_universeMap.contains(universe) == false)
        return false;

    QMutexLocker locker(&m_dataMutex);
    m_universeMap[universe].outputAddress = QHostAddress(address);

    if (m_ipAddr == QHostAddress::LocalHost)
        return QHostAddress(address) == QHostAddress::LocalHost;
    return QHostAddress(address) == QHostAddress::Null;
}

bool HUEController::setOutputPort(quint32 universe, quint16 port)
{
    if (m_universeMap.contains(universe) == false)
        return false;

    QMutexLocker locker(&m_dataMutex);
    if (m_universeMap.contains(universe))
        m_universeMap[universe].outputPort = port;

    return port == 80;
}
bool HUEController::setOutputUser(quint32 universe, QString user)
{
    if (m_universeMap.contains(universe) == false)
        return false;

    QMutexLocker locker(&m_dataMutex);
    if (m_universeMap.contains(universe))
        m_universeMap[universe].outputUser = user;

    return user == "newdeveloper";
}

QList<quint32> HUEController::universesList() const
{
    return m_universeMap.keys();
}

UniverseInfo* HUEController::getUniverseInfo(quint32 universe)
{
    if (m_universeMap.contains(universe))
        return &m_universeMap[universe];

    return NULL;
}

HUEController::Type HUEController::type() const
{
    int type = Unknown;
    foreach(UniverseInfo info, m_universeMap.values())
    {
        type |= info.type;
    }

    return Type(type);
}

quint32 HUEController::line() const
{
    return m_line;
}

quint64 HUEController::getPacketSentNumber() const
{
    return m_packetSent;
}
QString HUEController::getLastPacketSent() const
{
    return m_lastCmd;
}

quint64 HUEController::getPacketReceivedNumber() const
{
    return m_packetReceived;
}

quint16 HUEController::getHash(QString path)
{
    quint16 hash;
    if (m_hashMap.contains(path))
        hash = m_hashMap[path];
    else
    {
        /** No existing hash found. Add a new key to the table */
        hash = qChecksum(path.toUtf8().data(), path.length());
        m_hashMap[path] = hash;
    }

    return hash;
}

/*
void MainWindow::put4(QVariantMap postdata)
{
    QTcpSocket *socket = new QTcpSocket(this);

    socket->connectToHost("192.168.1.201", 80);

    if(socket->waitForConnected(1000))
    {
        qDebug() << "Connected!";
        QString baseString = "";
            baseString.append(QString("PUT /api/newdeveloper/lights/3/state HTTP/1.1\r\n").toUtf8());
            baseString.append(QString("192.168.1.201\r\n").toUtf8());
            baseString.append(QString("User-Agent: My app name v0.1\r\n").toUtf8());
            baseString.append(QString("X-Custom-User-Agent: My app name v0.1\r\n").toUtf8());
            baseString.append(QString("Content-Type: application/json\r\n").toUtf8());

            //QString jsonString = "{\"bri\":64,\"hue\":54675,\"on\":true,\"sat\":254,\"transitiontime\":1}";
            //QByteArray json = jsonString.toUtf8();
            QJsonDocument jsonDoc = QJsonDocument::fromVariant(postdata);
            QByteArray json = jsonDoc.toJson(QJsonDocument::Compact);

            baseString.append(QString("Content-Length:").toUtf8());
            baseString.append(QString::number(json.length()));
            baseString.append("\r\n").toUtf8();
            baseString.append(QString("\r\n").toUtf8());
            baseString.append(json);

            socket->write(baseString.toUtf8());
            socket->waitForBytesWritten(500);
        socket->waitForReadyRead(1000);

        qDebug() << "Reading: " << socket->bytesAvailable();
        qDebug() << socket->readAll();
        // close the connection
        socket->close();
    }
    else
    {
        qDebug() << "Not connected!";
    }
}
*/

void HUEController::sendDmx(const quint32 universe, const QByteArray &dmxData)
{
    QMutexLocker locker(&m_dataMutex);
    QByteArray dmxPacket;
    QHostAddress outAddress = QHostAddress::Null;
    quint32 outPort = 80;

    QString resHex;
    QString resDec;
    for (int i = 0; i < dmxData.size(); i++) {
      resHex.append( QString::number(dmxData.at(i), 16).rightJustified(2, '0') );
      if ( !resDec.isEmpty() )
         resDec.append( ", " );
      resDec.append( QString::number(dmxData.at(i) ));
    }

    if (m_universeMap.contains(universe))
    {
        outAddress = m_universeMap[universe].outputAddress;
        outPort = m_universeMap[universe].outputPort;
    }

    if (m_dmxValuesMap.contains(universe) == false)
        m_dmxValuesMap[universe] = new QByteArray(512, 0);
    QByteArray *dmxValues = m_dmxValuesMap[universe];

    for (int i = 0; i < dmxData.length(); i++) //4 canaux par peripherique
    { bool modif = false;
        int r,g,b,bri,light = i / 4 +1;
        r = dmxData[i];
        if  (dmxData[i] != dmxValues->at(i)) {
           dmxValues->replace(i, 1, (const char *)(dmxData.data() + i), 1);
           modif = true;
        }
        i++;
        g = dmxData[i];
        if  (dmxData[i] != dmxValues->at(i)) {
           dmxValues->replace(i, 1, (const char *)(dmxData.data() + i), 1);
           modif = true;
        }
        i++;
        b = dmxData[i];
        if  (dmxData[i] != dmxValues->at(i)) {
           dmxValues->replace(i, 1, (const char *)(dmxData.data() + i), 1);
           modif = true;
        }
        i++;
        bri = dmxData[i];
        if  (dmxData[i] != dmxValues->at(i)) {
           dmxValues->replace(i, 1, (const char *)(dmxData.data() + i), 1);
           modif = true;
        }

        if (modif){
            QColor c= QColor::fromRgb(r,g,b);
            quint16 hue = c.hue() * 65535 / 360;
            quint8 sat = c.saturation();


            //m_packetizer->setupHUEDmx(dmxPacket, universe, i, dmxData[i]);
            qint64 sent =0;

            m_outputSocket->connectToHost(outAddress, outPort);
            if(! m_outputSocket->waitForConnected(200))
            {
                qDebug() << "[HUE] sendDmx failed. Errno: " << m_outputSocket->error();
                qDebug() << "Errmgs: " << m_outputSocket->errorString();
                continue;
            }

            QString baseString = "";
                baseString.append(QString("PUT /api/newdeveloper/lights/").toUtf8());
                baseString.append(QString::number(light).toUtf8());
                baseString.append(QString("/state HTTP/1.1\r\n").toUtf8());
                baseString.append(outAddress.toString().toUtf8());
                //baseString.append(QString("\r\nUser-Agent: My app name v0.1\r\n").toUtf8());
                //baseString.append(QString("X-Custom-User-Agent: My app name v0.1\r\n").toUtf8());
                baseString.append(QString("Content-Type: application/json\r\n").toUtf8());

                QString jsonString = "{\"bri\":";
                jsonString.append(QString::number(bri));
                jsonString.append(QString(",\"hue\":"));
                jsonString.append(QString::number(hue));
                jsonString.append(QString(",\"on\":true,\"sat\":"));
                jsonString.append(QString::number(sat));
                jsonString.append(QString(",\"transitiontime\":0}"));
                m_lastCmd = QString::number(light) +" "+jsonString;
                m_lastCmd.append("<br>");
                m_lastCmd.append(resDec);
                QByteArray json = jsonString.toUtf8();
                //QJsonDocument jsonDoc = QJsonDocument::fromVariant(postdata);
                //QByteArray json = jsonDoc.toJson(QJsonDocument::Compact);

                baseString.append(QString("Content-Length:").toUtf8());
                baseString.append(QString::number(json.length()));
                baseString.append("\r\n").toUtf8();
                baseString.append(QString("\r\n").toUtf8());
                baseString.append(json);

                m_outputSocket->write(baseString.toUtf8());
                m_outputSocket->waitForBytesWritten(100);
                m_outputSocket->waitForReadyRead(100);

            //qDebug() << "Reading: " << socket->bytesAvailable();
            //qDebug() <<
                m_outputSocket->readAll();
            m_outputSocket->close();


            //TODO qint64 sent = m_outputSocket->writeDatagram(dmxPacket.data(), dmxPacket.size(),outAddress, outPort);
            if (sent < 0)
            {
                qDebug() << "[HUE] sendDmx failed. Errno: " << m_outputSocket->error();
                qDebug() << "Errmgs: " << m_outputSocket->errorString();
            }
            else
                m_packetSent++;
        }
    }
}

void HUEController::sendFeedback(const quint32 universe, quint32 channel, uchar value, const QString &key)
{
    QMutexLocker locker(&m_dataMutex);
    QHostAddress outAddress = QHostAddress::Null;
    //quint32 outPort = 9000 + universe;

    if (m_universeMap.contains(universe))
    {
        outAddress = m_universeMap[universe].feedbackAddress;
        //outPort = m_universeMap[universe].feedbackPort;
    }

    QString path = key;
    // on invalid key try to retrieve the HUE path from the hash table.
    // This works only if the HUE widget has been previously moved by the user
    if (key.isEmpty())
        path = m_hashMap.key(channel);

    qDebug() << "[HUE] sendFeedBack - Key:" << path << "value:" << value;

    QByteArray values;
    QByteArray huePacket;

    // multiple value path
    if (path.length() > 2 && path.at(path.length() - 2) == '_')
    {
        int valIdx = QString(path.at(path.length() - 1)).toInt();
        path.chop(2);
        if (m_universeMap[universe].multipartCache.contains(path) == false)
        {
            qDebug() << "[HUE] Multi-value path NOT in cache. Allocating default.";
            m_universeMap[universe].multipartCache[path] = QByteArray((int)2, (char)0);
        }

        values = m_universeMap[universe].multipartCache[path];
        if (values.count() <= valIdx)
            values.resize(valIdx + 1);
        values[valIdx] = (char)value;
        m_universeMap[universe].multipartCache[path] = values;

        //qDebug() << "Values to send:" << QString::number((uchar)values.at(0)) <<
        //            QString::number((uchar)values.at(1)) << values.count();
    }
    else
        values.append((char)value); // single value path

    QString pTypes;
    pTypes.fill('f', values.count());

    m_packetizer->setupHUEGeneric(huePacket, path, pTypes, values);
    qint64 sent = 0;
    // qint64 sent = m_outputSocket->writeDatagram(huePacket.data(), huePacket.size(), outAddress, outPort);
    if (sent < 0)
    {
        qDebug() << "[HUE] sendDmx failed. Errno: " << m_outputSocket->error();
        qDebug() << "Errmgs: " << m_outputSocket->errorString();
    }
    else
        m_packetSent++;
}

void HUEController::processPendingPackets()
{
    /*
    QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
    while (socket->hasPendingDatagrams())
    {
        QByteArray datagram;
        QHostAddress senderAddress;
        datagram.resize(socket->pendingDatagramSize());
        socket->readDatagram(datagram.data(), datagram.size(), &senderAddress);
        //if (senderAddress != m_ipAddr)
        {
            //qDebug() << "Received packet with size: " << datagram.size() << ", host: " << senderAddress.toString();
            QList< QPair<QString, QByteArray> > messages = m_packetizer->parsePacket(datagram);

            QListIterator <QPair<QString,QByteArray> > it(messages);
            while (it.hasNext() == true)
            {
                QPair <QString,QByteArray> msg(it.next());

                QString path = msg.first;
                QByteArray values = msg.second;

                //qDebug() << "[HUE] message has path:" << path << "values:" << values.count();
                if (values.isEmpty())
                    continue;

                quint16 port = socket->localPort();
                if (m_portsMap.contains(port))
                {
                    if (values.count() > 1)
                    {
                        quint32 uni = m_portsMap[port];
                        if (m_universeMap.contains(uni))
                        {
                            //m_universeMap[uni].multipartCache[path].resize(values.count());
                            m_universeMap[uni].multipartCache[path] = values;
                        }
                        for(int i = 0; i < values.count(); i++)
                        {
                            QString modPath = QString("%1_%2").arg(path).arg(i);
                            emit valueChanged(m_portsMap[port], m_line, getHash(modPath), (uchar)values.at(i), modPath);
                        }
                    }
                    else
                        emit valueChanged(m_portsMap[port], m_line, getHash(path), (uchar)values.at(0), path);
                }
            }
            m_packetReceived++;
        }
    } */
}
