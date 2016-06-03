/*
  Q Light Controller Plus
  huepacketizer.h

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

#include <QHostAddress>
#include <QByteArray>
#include <QString>
#include <QHash>
#include <QPair>

#ifndef HUEPACKETIZER_H
#define HUEPACKETIZER_H

class HUEPacketizer
{
    /*********************************************************************
     * Initialization
     *********************************************************************/
public:
    HUEPacketizer();
    ~HUEPacketizer();

    enum TagType { Integer = 0x01, Float = 0x02, Time = 0x03, String = 0x04, Blob = 0x05 };

public:
    /*********************************************************************
     * Sender functions
     *********************************************************************/

    /** Prepare an HUE DMX packet
 */
    /**
     * Prepare an HUE DMX message using a HUE path like
     * /$universe/dmx/$channel
     * All values are transmitted as float (HUE 'f')
     *
     * @param data the message composed by this function to be sent on the network
     * @param universe the universe used to compose the HUE message path
     * @param channel the DMX channel used to compose the HUE message path
     * @param value the value to be transmitted (as float)
     */
    void setupHUEDmx(QByteArray& data, quint32 universe, quint32 channel, uchar value);

    /**
     * Prepare an generic HUE message using the specified $path.
     * Values are appended to the message as specified by their $types.
     * Note that $types and $values must have the same length
     *
     * @param data the message composed by this function to be sent on the network
     * @param path the HUE path to be used in the message
     * @param types the list of types of the following $values
     * @param values the actual values to be appended to the message
     */
    void setupHUEGeneric(QByteArray& data, QString &path, QString types, QByteArray &values);

    /*********************************************************************
     * Receiver functions
     *********************************************************************/
private:
    /**
     * Extract an HUE message received from a buffer.
     * As a result, it returns the extracted HUE path and values
     * (empty if invalid)
     *
     * @param data the buffer containing the HUE message
     * @param path the HUE path extracted from the buffer
     * @param values the array of values extracted from the buffer
     * @return true on successful parsing, otherwise false
     */
    bool parseMessage(QByteArray &data, QString &path, QByteArray &values);
public:
    /**
     * Parse a HUE packet received from the network.
     *
     * @param data the payload of a UDP packet received from the network
     * @return a list of couples of HUE path/values
     */
    QList< QPair<QString, QByteArray> > parsePacket(QByteArray& data);

};

#endif
