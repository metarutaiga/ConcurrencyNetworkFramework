CXX=clang++
CXXFLAGS=-O3 -g -std=c++11 -stdlib=libc++

SRCS=Buffer.cpp Connection.cpp Framework.cpp Listener.cpp Log.cpp Socket.cpp
OBJS=$(SRCS:.cpp=.o)
DEPS=$(OBJS:.o=.d)
ConcurrencyNetworkFramework.a: $(OBJS)

TARGETS=ConcurrencyNetworkFramework.a

.PHONY: all
all: $(TARGETS)

.PHONY: clean
clean:
	$(RM) $(TARGETS)
	$(RM) $(OBJS)
	$(RM) $(DEPS)
