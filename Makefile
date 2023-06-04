TARGET = mysh
CC     = gcc
CFLAGS = -g -Wall -Wvla -fsanitize=address

$(TARGET): $(TARGET).c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -rf $(TARGET) *.o *.a *.dylib *.dSYM