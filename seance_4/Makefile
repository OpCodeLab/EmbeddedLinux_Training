.PHONY: always

all: lab1_time lab2_background	


time:
	gcc -o out/time time1.c
	clear
	out/time

namespace:
	gcc -o out/namespace namespace.c
	clear
	out/namespace

timer:
	gcc -o out/timer timer.c
	clear
	out/timer
	
timerfd:
	gcc -o out/timerfd timerfd.c
	clear
	out/timerfd
alarm:
	gcc -o out/alarm alarm.c
	clear
	out/alarm

inotify:
	gcc -o out/inotify inotify.c
	clear
	out/inotify  ./file_to_watch.txt

client_epoll:
	gcc -o out/client_epoll client_epoll.c
	clear
	out/client_epoll

ipc_lab_mulithread:
	gcc -o out/ipc_lab_mulithread ipc_lab_mulithread.c -lpthread
	clear
	out/ipc_lab_mulithread

server_epoll:
	gcc -o out/server_epoll server_epoll.c
	clear
	out/server_epoll

daemon: 
	gcc -o out/daemon  daemon.c
	clear
	out/daemon

clean:
	rm -r out/*	