BOOST:=-L$(BOOST_HOME)/lib -I$(BOOST_HOME)/include
LIBS:=$(LIBS) -lboost_serialization -lpthread
CFLAGS:=$(CFLAGS) -g -std=c++11

ifeq ($(origin CXX), undefined)
CXX:=g++-5
endif

all: *.hh test.cc
	$(CXX) $(CFLAGS) test.cc -o test $(BOOST) $(LIBS)

test: all
	./test
