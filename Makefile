yash: yash.c
	gcc -o yash yash.c -l readline

run: yash
	./yash

clean:
	rm -f yash