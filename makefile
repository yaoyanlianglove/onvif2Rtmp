SUB_DIR = main \
          onvif \
          save 
          
SUB_DIR += obj
          

all:$(SUB_DIR)
$(SUB_DIR):ECHO
	@make -C $@
ECHO:
	@echo **************begin compile**************
	@echo $(SUB_DIR)

.PHONY:clean

clean:
	make -C onvif clean
	make -C main  clean
	make -C save  clean 
	make -C obj   clean 
