NS=pfs_master
CC=gcc
CXX=g++
INC+= 
LIB+= -lpthread -ldl -lrt -rdynamic -L../lib -lpfs -lstdc++ -L../3rdlib -lnmshm
TARGET=$(NS)
ALL:$(TARGET)

CFLAGS+= -g -O2 -pipe -Wall -fno-strict-aliasing -D_GNU_SOURCE -D_REENTRANT -D_NS_=\"$(NS)\" -DPOSS -I../lib -I../3rdlib

OBJ= main.o	\
	daemon.o \
	fdinfo.o	\
	thread.o	\
	version.o	\
	watchdog.o	\
	pfs_init.o \
	pfs_task.o \
	pfs_time_stamp.o \
	pfs_tmp_status.o \
	pfs_localfile.o \
	pfs_agent.o \
	pfs_timer.o \
	pfs_del_file.o \
	pfs_file_filter.o \
	pfs_maintain.o \
	parsepara.o \
	pfs_so.o
		
$(TARGET):$(OBJ)
	$(CC)	$(CFLAGS) -o $@ $^ $(LIB)
#	install $(TARGET) ../bin
%.o: %.cpp
	$(CXX)  $(CFLAGS) $(INC) -c -o $@ $< 	 
%.o: %.c
	$(CC)  $(CFLAGS) $(INC) -c -o $@ $< 	 
clean:
	rm -f $(OBJ)
	rm -f $(TARGET) 	
