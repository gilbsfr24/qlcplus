/*
  Q Light Controller Plus
  hueplugin.h

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

#ifndef HUEPLUGIN_H
#define HUEPLUGIN_H

#include <QNetworkAddressEntry>
#include <QNetworkInterface>
#include <QHostAddress>
#include <QString>
#include <QHash>
#include <QFile>

#include "qlcioplugin.h"
#include "huecontroller.h"

typedef struct
{
    QString IPAddress;
    HUEController* controller;
} HUEIO;

#define HUE_INPUTPORT "inputPort"
#define HUE_FEEDBACKIP "feedbackIP"
#define HUE_FEEDBACKPORT "feedbackPort"
#define HUE_OUTPUTIP "outputIP"
#define HUE_OUTPUTPORT "outputPort"
#define HUE_TRANSMITMODE "transmitMode"
#define HUE_OUTPUTUSER "outputUser"


class HUEPlugin : public QLCIOPlugin
{
    Q_OBJECT
    Q_INTERFACES(QLCIOPlugin)
#if QT_VERSION > QT_VERSION_CHECK(5, 0, 0)
    Q_PLUGIN_METADATA(IID QLCIOPlugin_iid)
#endif

    /*********************************************************************
     * Initialization
     *********************************************************************/
public:
    /** @reimp */
    virtual ~HUEPlugin();

    /** @reimp */
    void init();

    /** @reimp */
    QString name();

    /** @reimp */
    int capabilities() const;

    /** @reimp */
    QString pluginInfo();

    /*********************************************************************
     * Outputs
     *********************************************************************/
public:
    /** @reimp */
    bool openOutput(quint32 output, quint32 universe);

    /** @reimp */
    void closeOutput(quint32 output, quint32 universe);

    /** @reimp */
    QStringList outputs();

    /** @reimp */
    QString outputInfo(quint32 output);

    /** @reimp */
    void writeUniverse(quint32 universe, quint32 output, const QByteArray& data);

    /*************************************************************************
     * Inputs
     *************************************************************************/
public:
    /** @reimp */
    bool openInput(quint32 input, quint32 universe);

    /** @reimp */
    void closeInput(quint32 input, quint32 universe);

    /** @reimp */
    QStringList inputs();

    /** @reimp */
    QString inputInfo(quint32 input);

    /** @reimp */
    void sendFeedBack(quint32 universe, quint32 input, quint32 channel, uchar value, const QString& key);

    /*********************************************************************
     * Configuration
     *********************************************************************/
public:
    /** @reimp */
    void configure();

    /** @reimp */
    bool canConfigure();

    /** @reimp */
    void setParameter(quint32 universe, quint32 line, Capability type, QString name, QVariant value);

    /** Get a list of the available Input/Output lines */
    QList<HUEIO> getIOMapping();

    void rescanDevices();

private:
    /** Map of the HUE plugin Input/Output lines */
    QList<HUEIO>m_IOmapping;
};

#endif
