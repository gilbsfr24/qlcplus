/*
  Q Light Controller Plus
  configurehue.h

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

#ifndef CONFIGUREHUE_H
#define CONFIGUREHUE_H

#include "ui_configurehue.h"

class HUEPlugin;

class ConfigureHUE : public QDialog, public Ui_ConfigureHUE
{
    Q_OBJECT

    /*********************************************************************
     * Initialization
     *********************************************************************/
public:
    ConfigureHUE(HUEPlugin* plugin, QWidget* parent = 0);
    virtual ~ConfigureHUE();

    /** @reimp */
    void accept();

public slots:
    int exec();

private:
    void slotRefreshClicked();
    void fillMappingTree();
    void showIPAlert(QString ip);

private:
    HUEPlugin* m_plugin;

};

#endif
