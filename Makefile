# Configuration

CC			= gcc
CFLAGS		= -g -std=gnu99 -Wall -Iinclude -fPIC -pthread 
PY_CFLAGS 	= -I/usr/include/python2.7
PY_LDFLAGS	= -lpython2.7
TARGET		= client/mycal server/mycalserver

# Rules
make: $(TARGET)

$(TARGET): client/clientpython_interpreter.c server/server.c
	$(CC) $(CFLAGS) $(PY_CFLAGS) -o client/mycal client/clientpython_interpreter.c $(PY_LDFLAGS)
	$(CC) $(CFLAGS) -o server/mycalserver server/server.c 

clean:
	@echo "Removing  executables"
	@rm -f client/mycal server/mycalserver
