
all: 
	gcc -Wall -pedantic -shared -fPIC -o cron.so cron.c
	gcc -Wall -pedantic -shared -fPIC -o tesseract.so tesseract.c -l tesseract
