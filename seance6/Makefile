.PHONY: always

all: 1-thread



lab35_threadpool: lab35_threadpool.c always
	gcc -g3 -o out/lab35_threadpool lab35_threadpool.c -lpthread
	clear
	time out/lab35_threadpool

lab_multithreading_debug: lab_multithreading_debug.c always
	gcc -g -O0 -Wall -o out/lab_multithreading_debug lab_multithreading_debug.c -lpthread
	clear
	out/lab_multithreading_debug

producer_consumer1: producer_consumer1.c always
	gcc -g -O0 -Wall -o out/producer_consumer1 producer_consumer1.c -lpthread
	clear
	out/producer_consumer1

producer_consumer2: producer_consumer2.c always
	gcc -g -O0 -Wall -o out/producer_consumer2 producer_consumer2.c -lpthread
	clear
	out/producer_consumer2
	
producer_consumer3: producer_consumer3.c always
	gcc -g -O0 -Wall -o out/producer_consumer3 producer_consumer3.c -lpthread
	clear
	out/producer_consumer3



clean:
	rm -r out/*	