.PHONY: always

all: 1-fork 2-wait

lab1_stdout_gprof: lab1_stdout_gprof.c always
	gcc -pg -o out/lab1_stdout_gprof lab1_stdout_gprof.c
	clear
	out/lab1_stdout_gprof 
	gprof out/lab1_stdout_gprof gmon.out > gprof.txt

lab1_stdout: lab1_stdout.c always
	gcc -pg -o out/lab1_stdout lab1_stdout.c
	clear
	out/lab1_stdout >stdout.txt  2>stderr.txt


lab2_background: lab2_background.c always
	gcc -o out/lab2_background lab2_background.c
	clear
	out/lab2_background

lab3_fork: lab3_fork.c always
	gcc -o out/lab3_fork lab3_fork.c
	clear
	out/lab3_fork

lab4_exit: lab4_exit.c always
	gcc -o out/lab4_exit lab4_exit.c
	clear
	out/lab4_exit

lab5_forkpid: lab5_forkpid.c always
	gcc -o out/lab5_forkpid lab5_forkpid.c
	clear
	out/lab5_forkpid

lab6_waitpid: lab6_waitpid.c always
	gcc -o out/lab6_waitpid lab6_waitpid.c
	clear
	out/lab6_waitpid

lab7_processmap: lab7_processmap.c always
	gcc -o out/lab7_processmap lab7_processmap.c
	clear
	out/lab7_processmap
	

lab8_cmdexec: lab8_cmdexec.c always
	gcc -o out/lab8_cmdexec lab8_cmdexec.c -lpthread
	clear
	out/lab8_cmdexec


lab9_signal: lab9_signal_post.c lab9_signal_wait.c always
	gcc -o out/lab9_signal_post lab9_signal_post.c
	gcc -o out/lab9_signal_wait lab9_signal_wait.c
	clear
	out/lab9_signal_wait &
	ps aux | grep lab9_signal_wait
	out/lab9_signal_post

lab10_sigfault : lab10_sigfault.c always
	gcc -o out/lab10_sigfault lab10_sigfault.c
	clear
	out/lab10_sigfault


lab11_sigint: lab11_sigint.c always
	gcc -o out/lab11_sigint lab11_sigint.c
	clear
	out/lab11_sigint


lab12_rlimit: lab12_rlimit.c always
	gcc -o out/lab12_rlimit lab12_rlimit.c
	clear
	out/lab12_rlimit

lab13_pipe: ipc/lab13_pipe.c always
	gcc -o out/lab13_pipe ipc/lab13_pipe.c
	clear
	out/lab13_pipe


lab14_queue: ipc/lab14_queue1.c ipc/lab14_queue2.c always
	gcc -o out/lab14_queue1 ipc/lab14_queue1.c
	gcc -o out/lab14_queue2 ipc/lab14_queue2.c
	clear
	out/lab14_queue1
	out/lab14_queue2

lab15_shm: ipc/lab15_shm1.c ipc/lab15_shm2.c always
	gcc -o out/lab15_shm1 ipc/lab15_shm1.c
	gcc -o out/lab15_shm2 ipc/lab15_shm2.c
	clear
	out/lab15_shm1 &
	out/lab15_shm2

lab16_sem: ipc/lab16_sem1.c ipc/lab16_sem2.c always
	gcc -o out/lab16_sem1 ipc/lab16_sem1.c -lpthread
	gcc -o out/lab16_sem2 ipc/lab16_sem2.c -lpthread
	clear
	out/lab16_sem1 &
	out/lab16_sem2
	
lab17_socket: ipc/lab17_socket1.c ipc/lab17_socket2.c always
	gcc -o out/lab17_socket1 ipc/lab17_socket1.c 
	gcc -o out/lab17_socket2 ipc/lab17_socket2.c 
	clear
	out/lab17_socket1 &
	out/lab17_socket2


clean:
	rm -r out/*	