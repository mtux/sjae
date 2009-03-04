# global options
HAVE_GCC {
       QMAKE_CXXFLAGS_RELEASE += -s -O2 -Wno-non-virtual-dtor -Wno-long-long -pedantic -Wconversion
       QMAKE_CXXFLAGS_DEBUG += -g3 -ggdb -O0 -Wno-non-virtual-dtor -Wno-long-long -pedantic -Wconversion
 }
 
CONFIG += qt thread debug_and_release build_all warn_on
QT += gui script
