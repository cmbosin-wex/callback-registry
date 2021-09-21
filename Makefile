CFLAGS = 

.SUFFIXES:  
.SUFFIXES:     .c .o

all: test_callbacks

test_callbacks: test_callbacks.c callbacks.o
	gcc $< -o test_callbacks callbacks.o

.c.o: .c
	gcc $(CFLAGS) -c $< 

clean:
	rm -rf *.o test_callbacks

test: test_callbacks
	./test_callbacks