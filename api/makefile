CC=gcc
CXX=g++
INC+= 
LIB+= -lpthread -ldl -lrt -rdynamic -L../lib -lpfs 
TARGET=test_upload_file test_del_file test_upload_file_set_path test_upload_file_set_path_by_dir pictool test_del_file_by_dir test_upload_file_set_group test_mod_file test_upload_file_set_path_by_dir_set_group imgtool test_upload_file_bname
ALL:$(TARGET)

CFLAGS+= -g -O2 -pipe -Wall -fno-strict-aliasing -D_GNU_SOURCE -D_REENTRANT  -I../lib -I../3rdlib

OBJ= test_upload_file.o
del_obj = test_del_file.o
set_path_obj = test_upload_file_set_path.o
set_path_by_dir_obj = test_upload_file_set_path_by_dir.o
set_path_by_dir_m_obj = pictool.o
test_del_file_by_dir_obj = test_del_file_by_dir.o
test_upload_file_set_group_obj = test_upload_file_set_group.o
test_mod_file_obj = test_mod_file.o
test_upload_file_set_path_by_dir_set_group_obj = test_upload_file_set_path_by_dir_set_group.o
imgtool_obj = imgtool.o
test_upload_file_bname_obj = test_upload_file_bname.o
		
test_upload_file:$(OBJ)
	$(CC)	$(CFLAGS) -o $@ $^ $(LIB)
test_del_file:$(del_obj)
	$(CC)	$(CFLAGS) -o $@ $^ $(LIB)
test_upload_file_set_path:$(set_path_obj)
	$(CC)	$(CFLAGS) -o $@ $^ $(LIB)
test_upload_file_set_path_by_dir:$(set_path_by_dir_obj)
	$(CC)	$(CFLAGS) -o $@ $^ $(LIB)
pictool:$(set_path_by_dir_m_obj)
	$(CC)	$(CFLAGS) -o $@ $^ $(LIB)
test_del_file_by_dir:$(test_del_file_by_dir_obj)
	$(CC)	$(CFLAGS) -o $@ $^ $(LIB)
test_upload_file_set_group:$(test_upload_file_set_group_obj)
	$(CC)	$(CFLAGS) -o $@ $^ $(LIB)
test_mod_file:$(test_mod_file_obj)
	$(CC)	$(CFLAGS) -o $@ $^ $(LIB)
imgtool:$(imgtool_obj)
	$(CC)	$(CFLAGS) -o $@ $^ $(LIB)
test_upload_file_bname:$(test_upload_file_bname_obj)
	$(CC)	$(CFLAGS) -o $@ $^ $(LIB)
test_upload_file_set_path_by_dir_set_group:$(test_upload_file_set_path_by_dir_set_group_obj)
	$(CC)	$(CFLAGS) -o $@ $^ $(LIB)
#	install $(TARGET) ../bin
%.o: %.cpp
	$(CXX)  $(CFLAGS) $(INC) -c -o $@ $< 	 
%.o: %.c
	$(CC)  $(CFLAGS) $(INC) -c -o $@ $< 	 
clean:
	rm -f *.o
	rm -f $(TARGET) 	
