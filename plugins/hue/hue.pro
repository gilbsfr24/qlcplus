include(../../variables.pri)

TEMPLATE = lib
LANGUAGE = C++
TARGET   = hue

QT      += network
greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG      += plugin
INCLUDEPATH += ../interfaces
DEPENDPATH  += ../interfaces

win32:QMAKE_LFLAGS += -shared

# This must be after "TARGET = " and before target installation so that
# install_name_tool can be run before target installation
macx:include(../../macx/nametool.pri)

target.path = $$INSTALLROOT/$$PLUGINDIR
INSTALLS   += target

TRANSLATIONS += HUE_de_DE.ts
TRANSLATIONS += HUE_es_ES.ts
TRANSLATIONS += HUE_fi_FI.ts
TRANSLATIONS += HUE_fr_FR.ts
TRANSLATIONS += HUE_it_IT.ts
TRANSLATIONS += HUE_nl_NL.ts
TRANSLATIONS += HUE_cz_CZ.ts
TRANSLATIONS += HUE_pt_BR.ts
TRANSLATIONS += HUE_ca_ES.ts
TRANSLATIONS += HUE_ja_JP.ts

HEADERS += ../interfaces/qlcioplugin.h
HEADERS += huepacketizer.h \
           huecontroller.h \
           hueplugin.h \
           configurehue.h

FORMS += configurehue.ui

SOURCES += ../interfaces/qlcioplugin.cpp
SOURCES += huepacketizer.cpp \
           huecontroller.cpp \
           hueplugin.cpp \
           configurehue.cpp

unix:!macx {
   metainfo.path   = $$INSTALLROOT/share/appdata/
   metainfo.files += qlcplus-hue.metainfo.xml
   INSTALLS       += metainfo 
}
