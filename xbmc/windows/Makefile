SRCS=GUIMediaWindow.cpp \
     GUIJukeboxWindow.cpp \
     GUIWindowDebugInfo.cpp \
     GUIWindowFileManager.cpp \
     GUIWindowHome.cpp \
     GUIWindowLoginScreen.cpp \
     GUIWindowPointer.cpp \
     GUIWindowScreensaver.cpp \
     GUIWindowScreensaverDim.cpp \
     GUIWindowStartup.cpp \
     GUIWindowSystemInfo.cpp \
     GUIWindowWeather.cpp \

LIB=windows.a

include ../../Makefile.include
-include $(patsubst %.cpp,%.P,$(patsubst %.c,%.P,$(SRCS)))
