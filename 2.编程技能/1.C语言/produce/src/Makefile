PRODUCE_SRC=$(wildcard *.c)
TOPDIR_SRC = $(TOPDIR)/../

all: release

release: clean code_infos
	$(CC) $(CFLAGS) $(LDFLAGS) -s ${PRODUCE_SRC} -o produce

code_infos:
	@svn info $(TOPDIR_SRC) > ccver
	@cat ccver | grep -E "Relative URL|URL" > ccdver
	@cat ccver | grep "Last Changed" >> ccdver
	@echo "" >> ccdver
	@echo -n "buildTime: " >> ccdver
	@date >> ccdver

debug: clean
	$(CC) $(CFLAGS) $(LDFLAGS) -g -DDEBUG ${PRODUCE_SRC} -o produce

clean:
	rm -rf produce *.o *~
