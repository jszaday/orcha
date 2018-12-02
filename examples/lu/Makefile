BOOST:=-L$(BOOST_HOME)/lib -I$(BOOST_HOME)/include -L../../lib -I../../include -I../..
LIBS:=$(LIBS) -lboost_serialization -lboost_thread -lpthread -lboost_system -lorcha
CFLAGS:=$(CFLAGS) -g -std=c++11 -DBOOST_ERROR_CODE_HEADER_ONLY -DBOOST_ALL_NO_LIB -DBOOST_ALL_DYN_LINK -DBOOST_LOG_DYN_LINK

CXX=mpicxx

all: lu.cc
	$(CXX) $(CFLAGS) lu.cc -o lu $(BOOST) $(LIBS)

test: all
	./lu

clean :
	rm ./lu
