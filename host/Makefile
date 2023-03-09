ROOT = ..

include ../Makefile.common

CC = g++

TARGET = host

all: $(TARGET)

$(TARGET): %: %.cpp
	$(CC) $(CFLAGS) $(DEFINES) -o $@ $< `dpu-pkg-config --cflags --libs dpu` -g

clean:
	rm -f $(TARGET) *.o