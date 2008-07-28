include(../plugin_pro.inc)

DEPENDPATH += . GeneratedFiles

QT += gui xml

# Input
HEADERS += accounts.h accountsoptions.h \
  ../../include/accounts_i.h \
  ../../include/options_i.h
FORMS += accountsoptions.ui
SOURCES += accounts.cpp accountsoptions.cpp
