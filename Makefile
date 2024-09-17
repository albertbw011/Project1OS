yash: yash.c jobs.c parsing.c
	gcc -g -o yash yash.c jobs.c parsing.c -l readline

run: yash
	./yash

clean:
	rm -f yash 