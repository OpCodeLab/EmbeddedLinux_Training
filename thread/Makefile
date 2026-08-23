.PHONY: always

all: 1-thread


lab18_thrCreation: lab18_thrCreation.c always
	gcc -o out/lab18_thrCreation lab18_thrCreation.c -lpthread
	clear
	out/lab18_thrCreation

lab18_thrFork: lab18_thrFork.c always
	gcc -o out/lab18_thrFork lab18_thrFork.c -lpthread
	clear
	out/lab18_thrFork

lab19_thrDetached: lab19_thrDetached.c always
	gcc -o out/lab19_thrDetached lab19_thrDetached.c -lpthread
	clear
	out/lab19_thrDetached

lab20_thrGetvalue: lab20_thrGetValue.c always
	gcc -o out/lab20_thrGetValue lab20_thrGetValue.c -lpthread
	clear
	out/lab20_thrGetValue

lab21_thread_racecond: lab21_thread_racecond.c always
	gcc -o out/lab21_thread_racecond lab21_thread_racecond.c -lpthread
	out/lab21_thread_racecond

lab22_thread_mutex: lab22_thread_mutex.c always
	gcc -o out/lab22_thread_mutex lab22_thread_mutex.c -lpthread
	out/lab22_thread_mutex

lab23_thread_spinlock: lab23_thread_spinlock.c always
	gcc -o out/lab23_thread_spinlock lab23_thread_spinlock.c -lpthread
	out/lab23_thread_spinlock

lab23_thread_spinlock_bad: lab23_thread_spinlock_bad.c always
	gcc -o out/lab23_thread_spinlock_bad lab23_thread_spinlock_bad.c -lpthread
	out/lab23_thread_spinlock_bad


lab24_thread_atomic: lab24_thread_atomic.c always
	gcc -o out/lab24_thread_atomic lab24_thread_atomic.c -lpthread
	out/lab24_thread_atomic

lab25_thread_sanitizer:lab21_thread_racecond.c always
	gcc -o out/lab25_thread_sanitizer lab21_thread_racecond.c -lpthread -fsanitize=thread
	clear
	out/lab25_thread_sanitizer

lab25_thread_deadlock: lab25_thread_deadlock.c always
	gcc -o out/lab25_thread_deadlock lab25_thread_deadlock.c -lpthread
	clear
	out/lab25_thread_deadlock
	
lab26_thread_trylock: lab26_thread_trylock.c always
	gcc -o out/lab26_thread_trylock lab26_thread_trylock.c -lpthread
	clear
	out/lab26_thread_trylock


lab27_thread_cond: lab27_thread_cond.c always
	gcc -o out/lab27_thread_cond lab27_thread_cond.c -lpthread
	clear
	out/lab27_thread_cond

lab28_thread_signal: lab28_thread_signal.c always
	gcc -o out/lab28_thread_signal lab28_thread_signal.c -lpthread
	clear
	out/lab28_thread_signal
	

lab29_thread_barrier: lab29_thread_barrier.c always
	gcc -o out/lab29_thread_barrier lab29_thread_barrier.c -lpthread
	clear
	out/lab29_thread_barrier

lab30_thread_affinity: lab30_thread_affinity.c always
	gcc -o out/lab30_thread_affinity lab30_thread_affinity.c -lpthread
	clear
	out/lab30_thread_affinity


lab31_mem_Weakconsistency: lab31_mem_Weakconsistency.c always
	gcc -o out/lab31_mem_Weakconsistency lab31_mem_Weakconsistency.c -lpthread
	clear
	out/lab31_mem_Weakconsistency

lab32_mem_Seqconsistency: lab32_mem_Seqconsistency.c always
	gcc -o out/lab32_mem_Seqconsistency lab32_mem_Seqconsistency.c -lpthread
	clear
	out/lab32_mem_Seqconsistency

lab33_falsesharing: lab33_falsesharing.c always
	gcc -o out/lab33_falsesharing lab33_falsesharing.c -lpthread
	time out/lab33_falsesharing

lab33_falsesharingWithPadding: lab33_falsesharingWithPadding.c always
	gcc -o out/lab33_falsesharingWithPadding lab33_falsesharingWithPadding.c -lpthread
	time out/lab33_falsesharingWithPadding

lab34_threadpool: lab34_threadpool.c always
	gcc -o out/lab34_threadpool lab34_threadpool.c -lpthread
	clear
	out/lab34_threadpool

lab35_threadpool: lab35_threadpool.c always
	gcc -o out/lab35_threadpool lab35_threadpool.c -lpthread
	clear
	out/lab35_threadpool


clean:
	rm -r out/*	