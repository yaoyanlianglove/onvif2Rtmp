MODULE    = onvif
CC        = gcc
SUB_DIR   = soap2C
SRC       = $(wildcard ./*.c) 
OBJ       = $(patsubst %.c, %.o, $(notdir $(SRC)))
OBJD      = $(SRC:.c=.d)
OBJ_DIR   = ../obj
CFLAGS    += 


all: $(SUB_DIR) DEP $(MODULE) REMOVE COPY 
$(SUB_DIR):ECHO
	make -C $@
ECHO:
	@echo SUB_DIR	

DEP : $(OBJD)
%.d : %.c
	@rm -f $@
	@$(CC) -MM  $< > $@.
	@sed 's,/($*/)/.o[ :]*,/1.o $@ : ,g' < $@. > $@
	@rm -f $@.
ifneq ($(MAKECMDGOALS),clean)
    -include $(OBJD)
endif
$(MODULE) : $(OBJ)
	@echo $(OBJ)
%.o : %.c
	$(CC) $(CFLAGS) -c $^
REMOVE:
	@-rm -f *.d *.gch
COPY:
	@-cp $(OBJ) $(OBJ_DIR)


.PHONY:clean

clean:
	make -C soap2C clean
	@-rm -f $(OBJ)
	@-rm -f *.d *.gch
