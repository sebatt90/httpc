http: http.o main.o 
	mkdir -p ./build
	clang -o ./build/http -L. -I.  -g -Wwarnings http.o main.o 

http.o: http.c
	clang -c http.c

main.o: main.c
	clang -c main.c

clean:
	rm -rf ./build
	rm *.o