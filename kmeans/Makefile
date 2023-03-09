ROOT = ..

include ../Makefile.common

CC = dpu-upmem-dpurte-clang

NR_TASKLETS = 11

TARGET := kmeans

TARGET_OBJS = kmeans.o

.PHONY:	all clean

all: $(TARGET)

$(TARGET): $(TARGET_OBJS) $(TMLIB)
	$(CC) -o $@ $(TARGET_OBJS) -DNR_TASKLETS=$(NR_TASKLETS) $(LDFLAGS)

kmeans.o: kmeans.c kmeans_macros.h

.c.o:
	$(CC) -DNR_TASKLETS=$(NR_TASKLETS) $(CPPFLAGS) $(CFLAGS) $(DEFINES) -c $<

clean:
	rm -f $(TARGET) $(TARGET).tmp* *.o *.s