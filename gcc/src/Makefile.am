AM_CPPFLAGS = ${libcurl_CFLAGS} ${jsonc_CFLAGS} ${common_CFLAGS} -std=c++11 -w

bin_PROGRAMS = thingplugtest
thingplugtest_SOURCES = thingplugtest.cpp
thingplugtest_LDADD = ${libcurl_LIBS} ${jsonc_LIBS} -lpaho-mqtt3cs -lpaho-mqtt3as -lmqttpp -lpthread
