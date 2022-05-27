all: TRON_mini.exe

%.exe:%.c
	@gcc $< -lpthread -O2 -o $@

clean:
	@rm *.exe