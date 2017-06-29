export base_cc_flag = -I$(ROOT_DIR) -I$(ROOT_DIR)/common -I$(BASE_DIR) -I$(ROOT_DIR)/processes
export base_ln_flag = \
	-L$(ROOT_DIR)/base/process -lprocess\
	-L$(ROOT_DIR)/base/net -lnet\
	-L$(ROOT_DIR)/base/component -lcomponent\

export dependent_base_files =\
	$(ROOT_DIR)/base/process/libprocess.a\
	$(ROOT_DIR)/base/net/libnet.a\
	$(ROOT_DIR)/base/component/libcomponent.a\
	/usr/local/mysql/lib/libmysqlclient.a\


SUBDIR =  \
		  game\



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
		ctags -R
