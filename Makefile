yash: yash.c jobs.c parsing.c
	gcc -std=c11 -o yash yash.c jobs.c parsing.c -l readline

run: yash
	./yash

clean:
	rm -f yash 