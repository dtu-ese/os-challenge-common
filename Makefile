default:
	gcc -O3 -o server server.c -lcrypto -pthread

clean:
	rm -f server