export ROOT_DIR = $(shell pwd)

export BASE_DIR = $(ROOT_DIR)/base

export CONF_DIR = $(ROOT_DIR)/config

export PROCESSES_DIR = $(ROOT_DIR)/processes

export optimization_flag = -O0

SUBDIR =  \
		  base\
		  processes\


all: targets

targets:
	@for subdir in $(SUBDIR); do  \
		$(MAKE) -C $$subdir || exit 1 ; \
		done 

clean:
	@for subdir in $(SUBDIR); do  \
		(cd $$subdir && $(MAKE) clean); \
		done

distclean:
	@for subdir in $(SUBDIR); do  \
		(cd $$subdir && $(MAKE) distclean); \
		done

ctags:
	ctags --exclude=libs -R
