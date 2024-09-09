yash.o: yash.c
    gcc -c yash.c -lreadline

clean:
    rm -f yash.o yash.c yash