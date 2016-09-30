mythread.a: mythread.o types.o
	ar rcs mythread.a mythread.o types.o
mythread.o: mythread.c
	gcc -c mythread.c
types.o: types.c
	gcc -c types.c
clean:
	rm -rf *.o mythread.a
