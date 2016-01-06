CC = gcc
RM = rm
CFLAGS = -Wall
# Macro definition
DEFS = -D OS_UNIX
CFLAGS += $(DEFS) -std=c99 -fPIC
# The target file
TARGET = libusbcan
# The source file
SRCS = usbcan.c
# The header file search path
INC = -I./include
# Dependent libraries,Select library file According to the operating system you using
LIBS = -L./lib/macos -lGinkgo_Driver -lpthread
# The target file
OBJS = $(SRCS:.c=.o)
# Link for the executable file
$(TARGET):$(OBJS)
	$(CC) $(INC) -dynamiclib -o $@.dylib $^ $(LIBS)

clean:
	$(RM) -rf $(TARGET) $(OBJS) *~ include/*~ *.so *.dylib

%.o:%.c
	$(CC) $(CFLAGS) $(INC) -o $@ -c $<
