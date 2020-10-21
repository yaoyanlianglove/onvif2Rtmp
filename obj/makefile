TARGET     = onvif2Rtmp.exe
CC         = gcc 
TARGET_DIR = ../bin
OBJ        = $(wildcard ./*.o) 

CFLAGS     += -L/usr/local/ssl/lib -lssl -lcrypto -lpthread -ldl

all:CHECK $(TARGET)
	@echo  "******************compile $(TARGET) done!******************"
	@cp $(TARGET) $(TARGET_DIR)
CHECK:
	@-rm -f $(TARGET)
	mkdir -p $(TARGET_DIR) 
$(TARGET):	
	$(CC) $(OBJ) $(CFLAGS) -o $@

.PHONY:clean

clean:
	@-rm -f $(OBJ)
	@-rm -f $(TARGET)
