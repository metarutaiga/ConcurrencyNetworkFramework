CXX=clang++
CXXFLAGS=-O3 -g -std=c++11 -stdlib=libc++

SRCS=Base.cpp Buffer.cpp Connection.cpp Framework.cpp Listener.cpp Log.cpp Socket.cpp
OBJS=$(SRCS:.cpp=.o)
DEPS=$(OBJS:.o=.d)
libConcurrencyNetworkFramework.a: $(OBJS)

TARGETS=libConcurrencyNetworkFramework.a

%.a: ; ar rcs $@ $^

.PHONY: all
all: $(TARGETS)

.PHONY: clean
clean:
	$(RM) $(TARGETS)
	$(RM) $(OBJS)
	$(RM) $(DEPS)
