MODULE  = rtsp2Rtmp.exe
CC      = g++
SRC     = $(wildcard ./*.cpp) 
OBJ     = $(patsubst %.cpp, %.o, $(notdir $(SRC)))
OBJD    = $(SRC:.cpp=.d)
BIN_DIR = ../bin
LIVEDIR = /opt/live555/live
CFLAGS   += -I$(LIVEDIR)/groupsock/include \
		    -I$(LIVEDIR)/liveMedia/include \
		    -I$(LIVEDIR)/UsageEnvironment/include \
		    -I$(LIVEDIR)/BasicUsageEnvironment/include \
		    -I/usr/local/include
LDFLAGS  += -L$(LIVEDIR)/liveMedia -lliveMedia \
			-L$(LIVEDIR)/BasicUsageEnvironment -lBasicUsageEnvironment \
			-L$(LIVEDIR)/groupsock -lgroupsock \
			-L$(LIVEDIR)/UsageEnvironment -lUsageEnvironment \
			-lrtmp \
			-L/usr/local/ssl/lib -lssl -lcrypto -lpthread -ldl
			

all : DEP $(MODULE) REMOVE COPY 
	@echo  "compile $(MODULE) done!"
DEP : $(OBJD)
	@echo $(OBJD)
%.d : %.cpp
	@rm -f $@
	@$(CC) -MM  $< > $@.
	@sed 's,/($*/)/.o[ :]*,/1.o $@ : ,g' < $@. > $@
	@rm -f $@.
ifneq ($(MAKECMDGOALS),clean)
    -include $(OBJD)
endif
$(MODULE) : $(OBJ)
	$(CC) $(OBJ) $(LDFLAGS) -o $@
%.o : %.cpp
	$(CC)  -c $^ $(CFLAGS)
REMOVE:
	@-rm -f *.d *.gch
COPY:
	@-cp $(MODULE) $(BIN_DIR)

.PHONY:clean

clean:
	@-rm -f $(OBJ)
	@-rm -f *.d *.gch *.exe
	@-rm -f $(BIN_DIR)/$(MODULE)




