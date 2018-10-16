BOOST:=-L$(BOOST_HOME)/lib -I$(BOOST_HOME)/include
LIBS:=$(LIBS) -lboost_serialization -lpthread
CFLAGS:=$(CFLAGS) -g -std=c++11

CXX=mpicxx

all: *.hh test.cc comm.cc core.cc distrib.cc
	$(CXX) $(CFLAGS) test.cc comm.cc core.cc distrib.cc -o test $(BOOST) $(LIBS)

test: all
	./test
