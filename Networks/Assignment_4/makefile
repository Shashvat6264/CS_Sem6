main: user1.o librsocket.a user2.o 
	gcc user1.o -L. -lrsocket -pthread -o user1
	gcc user2.o -L. -lrsocket -pthread -o user2

user1.o: user1.c rsocket.h
	gcc -c user1.c 

user2.o: user2.c rsocket.h
	gcc -c user2.c

rsocket.o: rsocket.c rsocket.h
	gcc -c rsocket.c -pthread

clean:
	rm *.a *.gch *.o user1 user2