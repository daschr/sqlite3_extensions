
all: 
	gcc -Wall -pedantic -shared -fPIC -o cron.so cron.c 
