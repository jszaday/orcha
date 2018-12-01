BOOST:=-L$(BOOST_HOME)/lib -I$(BOOST_HOME)/include -I$(CURDIR)/include -I$(CURDIR)
LIBS:=$(LIBS) -lboost_serialization -lboost_thread -lpthread -lboost_system
CFLAGS:=$(CFLAGS) -g -std=c++11 -DBOOST_ERROR_CODE_HEADER_ONLY -DBOOST_ALL_NO_LIB -DBOOST_ALL_DYN_LINK -DBOOST_LOG_DYN_LINK

CXX=mpicxx

SRCS=$(wildcard src/*.cc)

OBJS=$(SRCS:.cc=.o)

%.o : %.cc
	$(CXX) -c $(CFLAGS) $< -o $@ $(BOOST) $(LIBS)

lib/liborcha.a : $(OBJS)
	mkdir -p lib
	ar crf lib/liborcha.a $(OBJS)

all : lib/liborcha.a

clean :
	rm $(OBJS) lib/liborcha.a
