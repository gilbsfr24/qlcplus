/*
  Q Light Controller Plus
  huecontroller.h

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

#ifndef HUECONTROLLER_H
#define HUECONTROLLER_H

#include "huepacketizer.h"

#include <QScopedPointer>
#include <QtNetwork>
#include <QObject>
#include <QHash>
#include <QMap>

typedef struct
{
    QTcpSocket *inputSocket;

    quint16 inputPort;

    QHostAddress feedbackAddress;
    quint16 feedbackPort;

    QHostAddress outputAddress;
    quint16 outputPort;
    int outputTransmissionMode;
    QString outputUser;

    // cache of the HUE paths with multiple values, used to correctly
    // handle the flow of input and feedback values
    QHash<QString, QByteArray> multipartCache;
    int type;
} UniverseInfo;

class HUEController : public QObject
{
    Q_OBJECT

    /*********************************************************************
     * Initialization
     *********************************************************************/
public:
    enum Type { Unknown = 0x0, Input = 0x01, Output = 0x02 };

    enum TransmissionMode { twoChannels, fourChannels};

    HUEController(QString ipaddr,
                   Type type, quint32 line, QObject *parent = 0);

    ~HUEController();

    /** Return the controller IP address */
    QHostAddress getNetworkIP() const;

    /** Add a universe to the map of this controller */
    void addUniverse(quint32 universe, Type type);

    /** Remove a universe from the map of this controller */
    void removeUniverse(quint32 universe, Type type);

    /** Set a specific port to be bound to receive data from the
     *  given universe.
     *  Return true if this restores default input port */
    bool setInputPort(quint32 universe, quint16 port);

    /** Set a specific feedback IP address for the given QLC+ universe.
     *  Return true if this restores default feedback IP address */
    bool setFeedbackIPAddress(quint32 universe, QString address);

    /** Set a specific port where to send feedback packets
     *  for the given universe.
     *  Return true if this restores default feedback port */
    bool setFeedbackPort(quint32 universe, quint16 port);

    /** Set a specific output IP address for the given QLC+ universe.
     *  Return true if this restores default output IP address */
    bool setOutputIPAddress(quint32 universe, QString address);

    /** Set a specific port where to send outgoing packets
     *  for the given universe.
     *  Return true if this restores default output port */
    bool setOutputPort(quint32 universe, quint16 port);

// TODO: commentaire
    /** Set the transmission mode of the ArtNet DMX packets over the network.
     *  It can be 'Full', which transmits always 512 channels, or
     *  'Partial', which transmits only the channels actually used in a
     *  universe */
    bool setOutputTransmissionMode(quint32 universe, TransmissionMode mode);

    /** Converts a TransmissionMode value into a human readable string */
    static QString transmissionModeToString(TransmissionMode mode);

    /** Converts a human readable string into a TransmissionMode value */
    static TransmissionMode stringToTransmissionMode(const QString& mode);

    bool setOutputUser(quint32 universe, QString user);

    /** Return the list of the universes handled by
     *  this controller */
    QList<quint32> universesList() const;

    /** Return the specific information for the given universe */
    UniverseInfo* getUniverseInfo(quint32 universe);

    /** Return the global type of this controller */
    Type type() const;

    /** Return the plugin line associated to this controller */
    quint32 line() const;

    /** Get the number of packets sent by this controller */
    quint64 getPacketSentNumber() const;

    /** Get the number of packets received by this controller */
    quint64 getPacketReceivedNumber() const;

    QString getLastPacketSent() const;

    /** Send DMX data to a specific universe */
    void sendDmx(const quint32 universe, const QByteArray& dmxData);

    /** Send a feedback using the specified path and value */
    void sendFeedback(const quint32 universe, quint32 channel, uchar value, const QString &key);

protected:
    /** Calculate a 16bit unsigned hash as a unique representation
     *  of a HUE path. If new, the hash is added to the hash map (m_hashMap) */
    quint16 getHash(QString path);

private slots:
    /** Async event raised when new packets have been received */
    void processPendingPackets();

signals:
    void valueChanged(quint32 universe, quint32 input, quint32 channel, uchar value, QString key);

private:
    /** The controller IP address as QHostAddress */
    QHostAddress m_ipAddr;

    quint64 m_packetSent;
    quint64 m_packetReceived;
    QString m_lastCmd;

    /** QLC+ line to be used when emitting a signal */
    quint32 m_line;

    /** The UDP socket used to output HUE packets */
    QTcpSocket *m_outputSocket;

    /** Helper class used to create or parse HUE packets */
    QScopedPointer<HUEPacketizer> m_packetizer;

    /** Keeps the current dmx values to send only the ones that changed */
    /** It holds values for all the handled universes */
    QMap<quint32, QByteArray *> m_dmxValuesMap;

    /** Map of the QLC+ universes transmitted/received by this
     *  controller, with the related, specific parameters */
    QMap<quint32, UniverseInfo> m_universeMap;

    /** Map of the relation between input ports and QLC+ universes,
     *  since a single controller can handle input for several universes */
    QMap<quint16, quint32>m_portsMap;

    /** Mutex to handle the change of output IP address or in general
     *  variables that could be used to transmit/receive data */
    QMutex m_dataMutex;

    /** This is fundamental for the HUE controller. Every time a HUE signal is received,
      * the controller will calculate a 16 bit checksum of the HUE path and add it to
      * this hash table if new, otherwise the controller will use the hash table
      * to quickly retrieve a unique channel number
      */
    QHash<QString, quint16> m_hashMap;
};

#endif