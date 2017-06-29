objects = \
		  register_tcp_msg_handler.o\
		  register_timer_handler.o\
		  game.o\
		  coffeenet/coffeenet_db.o\
		  coffeenet/coffeenet_checker.o\
		  coffeenet/coffeenet.o\
		  coffeenet/coffeenet_manager.o\
		  user/user_manager.o\
		  user/user_db.o\
		  user/user.o\
		  fight/fight_db.o\
		  fight/team_manager.o\
		  fight/fight.o\
		  fight/area_fight.o\
		  fight/fight_manager.o\
		  fight/fight_rule.o\
		  fight/fight_rule_single_fail.o\
		  main.o\

cc = g++

cc_flags = -std=c++11 -ggdb -Wall -Werror \
		   $(optimization_flag)\
		   $(base_cc_flag)\

ln_flags = -Wl,-dn\
		   $(base_ln_flag)\
		   -Wl,-dy\
		   -lpthread\
		   -L/usr/local/mysql/lib/ -lmysqlclient\

target_ln_dependents = $(dependent_base_files)\

target = game_exec

.PHONY: all
all: $(target)

$(target) : $(objects) $(target_ln_dependents)
	@#此行用于调试   @set -e -x; \ #此行用于调试
	g++ $(objects) $(ln_flags) -o $(target) 

%.o: %.cpp
	$(cc) $(cc_flags) -c $< -o $@

%.d: %.cpp
	@#此行用于调试	@set -e -x; \ #此行用于调试
	@rm -f $@; \
		$(cc) $(cc_flags) -MM $< > $@.$$$$;  \
		sed -r 's,^(.*)\.o:,$@ $*\.o:,' $@.$$$$ > $@;  \
		rm -f $@.$$$$

-include $(objects:.o=.d)

.PHONY: clean distclean
clean:
	rm -f $(objects) $(objects:.o=.d) *.d *.d.* 
	rm -f $(target)

distclean: clean
