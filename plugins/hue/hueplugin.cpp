/*
  Q Light Controller Plus
  hueplugin.cpp

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

#include "hueplugin.h"
#include "configurehue.h"

#include <QSettings>
#include <QDebug>

HUEPlugin::~HUEPlugin()
{
}

void HUEPlugin::init()
{
    foreach(QNetworkInterface interface, QNetworkInterface::allInterfaces())
    {
        foreach (QNetworkAddressEntry entry, interface.addressEntries())
        {
            QHostAddress addr = entry.ip();
            if (addr.protocol() != QAbstractSocket::IPv6Protocol)
            {
                HUEIO tmpIO;
                tmpIO.IPAddress = entry.ip().toString();
                tmpIO.controller = NULL;

                bool alreadyInList = false;
                for(int j = 0; j < m_IOmapping.count(); j++)
                {
                    if (m_IOmapping.at(j).IPAddress == tmpIO.IPAddress)
                    {
                        alreadyInList = true;
                        break;
                    }
                }
                if (alreadyInList == false)
                {
                    m_IOmapping.append(tmpIO);
                }
            }
        }
    }
}

QString HUEPlugin::name()
{
    return QString("HUE");
}

int HUEPlugin::capabilities() const
{
    return QLCIOPlugin::Output | QLCIOPlugin::Infinite ;
    // | QLCIOPlugin::Input | QLCIOPlugin::Feedback | QLCIOPlugin::Infinite;
}

QString HUEPlugin::pluginInfo()
{
    QString str;

    str += QString("<HTML>");
    str += QString("<HEAD>");
    str += QString("<TITLE>%1</TITLE>").arg(name());
    str += QString("</HEAD>");
    str += QString("<BODY>");

    str += QString("<P>");
    str += QString("<H3>%1</H3>").arg(name());
    str += tr("This plugin provides input for devices supporting the HUE transmission protocol.");
    str += QString("</P>");

    return str;
}

/*********************************************************************
 * Outputs
 *********************************************************************/
QStringList HUEPlugin::outputs()
{
    QStringList list;
    int j = 0;

    init();

    foreach (HUEIO line, m_IOmapping)
    {
        list << QString("%1: %2").arg(j + 1).arg(line.IPAddress);
        j++;
    }
    return list;
}

QString HUEPlugin::outputInfo(quint32 output)
{
    if (output >= (quint32)m_IOmapping.length())
        return QString();

    QString str;

    str += QString("<H3>%1 %2</H3>").arg(tr("Output")).arg(outputs()[output]);
    str += QString("<P>");
    HUEController *ctrl = m_IOmapping.at(output).controller;
    if (ctrl == NULL || ctrl->type() == HUEController::Input)
        str += tr("Status: Not open");
    else
    {
        str += tr("Status: Open");
    //    str += QString("Status: Open %1").arg(tr(ctrl->))
        str += QString("<BR>");
        str += tr("Packets sent: ");
        str += QString("%1").arg(ctrl->getPacketSentNumber());

        str += QString("<BR>");
        str += tr("Last Packet Send: ");
        str += ctrl->getLastPacketSent();
    }
    str += QString("</P>");
    str += QString("</BODY>");
    str += QString("</HTML>");

    return str;
}

bool HUEPlugin::openOutput(quint32 output, quint32 universe)
{
    init();

    if (output >= (quint32)m_IOmapping.length())
        return false;

    qDebug() << "[HUE] Open output with address :" << m_IOmapping.at(output).IPAddress;

    // if the controller doesn't exist, create it
    if (m_IOmapping[output].controller == NULL)
    {
        HUEController *controller = new HUEController(m_IOmapping.at(output).IPAddress,
                                                        HUEController::Output, output, this);
        m_IOmapping[output].controller = controller;
    }

    m_IOmapping[output].controller->addUniverse(universe, HUEController::Output);
    addToMap(universe, output, Output);

    return true;
}

void HUEPlugin::closeOutput(quint32 output, quint32 universe)
{
    if (output >= (quint32)m_IOmapping.length())
        return;

    removeFromMap(output, universe, Output);
    HUEController *controller = m_IOmapping.at(output).controller;
    if (controller != NULL)
    {
        controller->removeUniverse(universe, HUEController::Output);
        if (controller->universesList().count() == 0)
        {
            delete m_IOmapping[output].controller;
            m_IOmapping[output].controller = NULL;
        }
    }
}

void HUEPlugin::writeUniverse(quint32 universe, quint32 output, const QByteArray &data)
{
    if (output >= (quint32)m_IOmapping.count())
        return;

    HUEController *controller = m_IOmapping[output].controller;
    if (controller != NULL) {
        //QByteArray wholeuniverse(512, 0);
        //wholeuniverse.replace(0, data.length(), data);
        //controller->sendDmx(universe, wholeuniverse);
        controller->sendDmx(universe, data);
      }
}

/*************************************************************************
  * Inputs
  *************************************************************************/  
QStringList HUEPlugin::inputs()
{
    QStringList list;
//    int j = 0;

    init();

//    foreach (HUEIO line, m_IOmapping)
//    {
//        list << QString("%1: %2").arg(j + 1).arg(line.IPAddress);
//        j++;
//    }
    return list;
}

bool HUEPlugin::openInput(quint32 input, quint32 universe)
{
    init();

    if (input >= (quint32)m_IOmapping.length())
        return false;

    qDebug() << "[HUE] Open input on address :" << m_IOmapping.at(input).IPAddress;

    // if the controller doesn't exist, create it
    if (m_IOmapping[input].controller == NULL)
    {
        HUEController *controller = new HUEController(m_IOmapping.at(input).IPAddress,
                                                        HUEController::Input, input, this);
        connect(controller, SIGNAL(valueChanged(quint32,quint32,quint32,uchar,QString)),
                this, SIGNAL(valueChanged(quint32,quint32,quint32,uchar,QString)));
        m_IOmapping[input].controller = controller;
    }

    m_IOmapping[input].controller->addUniverse(universe, HUEController::Input);
    addToMap(universe, input, Input);

    return true;
}

void HUEPlugin::closeInput(quint32 input, quint32 universe)
{
    if (input >= (quint32)m_IOmapping.length())
        return;

    removeFromMap(input, universe, Input);
    HUEController *controller = m_IOmapping.at(input).controller;
    if (controller != NULL)
    {
        controller->removeUniverse(universe, HUEController::Input);
        if (controller->universesList().count() == 0)
        {
            delete m_IOmapping[input].controller;
            m_IOmapping[input].controller = NULL;
        }
    }
}

QString HUEPlugin::inputInfo(quint32 input)
{
    if (input >= (quint32)m_IOmapping.length())
        return QString();

    QString str;

    str += QString("<H3>%1 %2</H3>").arg(tr("Input")).arg(inputs()[input]);
    str += QString("<P>");
    HUEController *ctrl = m_IOmapping.at(input).controller;
    if (ctrl == NULL || ctrl->type() == HUEController::Output)
        str += tr("Status: Not open");
    else
    {
        str += tr("Status: Open");
        str += QString("<BR>");
        str += tr("Packets received: ");
        str += QString("%1").arg(ctrl->getPacketReceivedNumber());
    }
    str += QString("</P>");
    str += QString("</BODY>");
    str += QString("</HTML>");

    return str;
}

void HUEPlugin::sendFeedBack(quint32 universe, quint32 input,
                             quint32 channel, uchar value, const QString &key)
{
    if (input >= (quint32)m_IOmapping.count())
        return;

    HUEController *controller = m_IOmapping[input].controller;
    if (controller != NULL)
        controller->sendFeedback(universe, channel, value, key);
}

/*********************************************************************
 * Configuration
 *********************************************************************/
void HUEPlugin::configure()
{
    ConfigureHUE conf(this);
    conf.exec();
}

bool HUEPlugin::canConfigure()
{
    return true;
}

void HUEPlugin::setParameter(quint32 universe, quint32 line, Capability type,
                              QString name, QVariant value)
{
    if (line >= (quint32)m_IOmapping.length())
        return;

    HUEController *controller = m_IOmapping.at(line).controller;
    if (controller == NULL)
        return;

    // If the Controller parameter is restored to its default value,
    // unset the corresponding plugin parameter
    bool unset;

    if (name == HUE_INPUTPORT)
        unset = controller->setInputPort(universe, value.toUInt());
    else if (name == HUE_FEEDBACKIP)
        unset = controller->setFeedbackIPAddress(universe, value.toString());
    else if (name == HUE_FEEDBACKPORT)
        unset = controller->setFeedbackPort(universe, value.toUInt());
    else if (name == HUE_OUTPUTIP)
        unset = controller->setOutputIPAddress(universe, value.toString());
    else if (name == HUE_OUTPUTPORT)
        unset = controller->setOutputPort(universe, value.toUInt());
    else if (name == HUE_TRANSMITMODE)
        unset = controller->setOutputTransmissionMode(universe, HUEController::stringToTransmissionMode(value.toString()));
    else if (name == HUE_OUTPUTUSER)
        unset = controller->setOutputUser(universe, value.toString());
    else
    {
        qWarning() << Q_FUNC_INFO << name << "is not a valid HUE parameter";
        return;
    }

    if (unset)
        QLCIOPlugin::unSetParameter(universe, line, type, name);
    else
        QLCIOPlugin::setParameter(universe, line, type, name, value);
}

QList<HUEIO> HUEPlugin::getIOMapping()
{
    return m_IOmapping;
}

/*****************************************************************************
 * Devices
 *****************************************************************************/

void HUEPlugin::rescanDevices()
{
}
/*****************************************************************************
 * Plugin export
 ****************************************************************************/
#if QT_VERSION < QT_VERSION_CHECK(5, 0, 0)
Q_EXPORT_PLUGIN2(HUEPlugin, HUEPlugin)
#endif
