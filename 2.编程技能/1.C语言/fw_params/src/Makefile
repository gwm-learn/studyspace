
EXEC = fw_params
EXEC_1 = fw_printparams
EXEC_2 = fw_setparams
SRCS = $(wildcard *.c)
OBJS = $(patsubst %.c,%.o,$(SRCS))

all: release

release: $(EXEC)
	ln -s $(EXEC) $(EXEC_1)
	ln -s $(EXEC) $(EXEC_2)

$(EXEC): $(OBJS)
	$(CC) -Wall -Werror $(CFLAGS) -o $@ $(OBJS)

clean:
	-rm -f $(EXEC) $(EXEC_1) $(EXEC_1) *.elf *.gdb *.o

