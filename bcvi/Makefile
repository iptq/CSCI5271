# Using the default compiler in Ubuntu 16.04:
# gcc (Ubuntu 5.4.0-6ubuntu1~16.04.4) 5.4.0 20160609
CC := gcc

# -g: include debugging information
# -m32: compile a 32-bit binary, even on a 64-bit machine
# -m64: compile a 64-bit binary
# -Wall: enable most warning messages
# -fno-stack-protector: disable a security mechanism
# -z execstack: allow code execution on the stack
# -z norelro: disable a security mechanism
BC_CFLAGS := -g -Wall -fno-stack-protector -z execstack -z norelro
NC_CFLAGS := -I/usr/include/ncursesw
NC_LIBS := -lncursesw -ltinfo

all: bcvi bcvi64 sudobcvi sudobcvi64

bcvi: bcvi.c
	$(CC) -m32 $(BC_CFLAGS) $(NC_CFLAGS) $< -o $@ $(NC_LIBS)

bcecho: bcecho.c
	$(CC) -m32 $(BC_CFLAGS) $(NC_CFLAGS) $< -o $@ $(NC_LIBS)

sudobcvi: bcvi.c
	$(CC) -m32 $(BC_CFLAGS) $(NC_CFLAGS) $< -o $@ $(NC_LIBS)

bcvi64: bcvi.c
	$(CC) -m64 $(BC_CFLAGS) $(NC_CFLAGS) $< -o $@ $(NC_LIBS)

sudobcvi64: bcvi.c
	$(CC) -m64 $(BC_CFLAGS) $(NC_CFLAGS) $< -o $@ $(NC_LIBS)

ifeq ($(shell which rootshell),/bin/rootshell)

EXE:=bcvi
SUDOEXE:=sudo$(EXE)
EXEPATH:=/usr/bin/$(EXE)
SUDOEXEPATH:=/usr/bin/sudo$(EXE)

install:
	@# The bcvi executable itself is not setuid or setgid
	sudo -v && sudo cp $(EXE) $(EXEPATH)
	sudo -v && sudo chown root:root $(EXEPATH)
	sudo -v && sudo chmod a+rx $(EXEPATH)
	sudo -v && sudo cp $(EXE)64 $(EXEPATH)64
	sudo -v && sudo chown root:root $(EXEPATH)64
	sudo -v && sudo chmod a+rx $(EXEPATH)64
	@# The sudobcvi executable is setuid root
	sudo -v && sudo cp $(SUDOEXE) $(SUDOEXEPATH)
	sudo -v && sudo chown root:root $(SUDOEXEPATH)
	sudo -v && sudo chmod a+rx $(SUDOEXEPATH)
	sudo -v && sudo chmod u+s $(SUDOEXEPATH)
	sudo -v && sudo cp $(SUDOEXE)64 $(SUDOEXEPATH)64
	sudo -v && sudo chown root:root $(SUDOEXEPATH)64
	sudo -v && sudo chmod a+rx $(SUDOEXEPATH)64
	sudo -v && sudo chmod u+s $(SUDOEXEPATH)64

else
install:
	@echo "Don't install this on a real machine!"
endif
