MODULE  = save
CC      = gcc
SRC     = $(wildcard ./*.c) 
OBJ     = $(patsubst %.c, %.o, $(notdir $(SRC)))
OBJD    = $(SRC:.c=.d)
OBJ_DIR = ../obj
CFLAGS  +=



all : DEP $(MODULE) REMOVE COPY 
	@echo  "compile $(MODULE) done!"
DEP : $(OBJD)
	@echo $(OBJD)
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
	@-rm -f $(OBJ)
	@-rm -f *.d *.gch




