merepc: $(wildcard source/*.c)
	@gcc -Wall -Wextra -I source $^ -o merepc -lX11

clean:
	@rm -f merepc

fresh: clean merepc
