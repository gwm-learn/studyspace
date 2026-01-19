EXEC = mobile-mngr

# 源文件路径
MODULES_SRCS = $(wildcard modules/src/*.c)
SERVICES_SRCS = $(wildcard services/src/*.c)
SRCS = $(MODULES_SRCS) $(SERVICES_SRCS)

# 目标文件
OBJS = $(patsubst %.c,%.o,$(SRCS))

all: $(EXEC)

$(EXEC): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $(OBJS) -luci -lm -lpthread -lqlril -lqlnet -lsuci

clean:
	-rm -f $(EXEC) *.elf *.gdb
	-rm -f modules/src/*.o services/src/*.o

.PHONY: all clean install