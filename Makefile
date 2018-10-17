BOOST:=-L$(BOOST_HOME)/lib -I$(BOOST_HOME)/include
LIBS:=$(LIBS) -lboost_serialization -lboost_thread -lpthread -lboost_system
CFLAGS:=$(CFLAGS) -g -std=c++11 -DBOOST_ERROR_CODE_HEADER_ONLY -DBOOST_ALL_NO_LIB -DBOOST_ALL_DYN_LINK -DBOOST_LOG_DYN_LINK

CXX=mpicxx

all: *.hh test.cc comm.cc core.cc distrib.cc
	$(CXX) $(CFLAGS) test.cc comm.cc core.cc distrib.cc -o test $(BOOST) $(LIBS)

test: all
	./test
