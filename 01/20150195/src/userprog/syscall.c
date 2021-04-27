#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "devices/shutdown.h"
#include "userprog/process.h"
#include "devices/input.h"
#include "threads/malloc.h"
static void syscall_handler (struct intr_frame *);
void
syscall_init (void) 
{
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{
	switch(*(uint32_t*)(f->esp)){
		case SYS_HALT:
			halt();
			break;
		case SYS_EXIT:
			if(!is_user_vaddr(f->esp+4)) exit(-1);
			exit(*(uint32_t*)(f->esp+4));
			break;
		case SYS_EXEC:
			if(!is_user_vaddr(f->esp+4)) exit(-1);
			f->eax = exec(*(char**)(f->esp+4));
			break;
		case SYS_WAIT:
			if(!is_user_vaddr(f->esp+4))
				exit(-1);
			f->eax = wait(*(int*)(f->esp+4));
			break;
		case SYS_READ:
			if(!is_user_vaddr(f->esp+4)) exit(-1);
			f->eax = read((int)*(uint32_t*)(f->esp+4), (void*)*(uint32_t*)(f->esp+8), (unsigned)*(uint32_t*)(f->esp+12));
			break;
		case SYS_WRITE:
			if(!is_user_vaddr(f->esp+4)) exit(-1);
			f->eax = write((int)*(uint32_t*)(f->esp+4), (void*)*(uint32_t*)(f->esp+8), (unsigned)*((uint32_t*)(f->esp+12)));
			break;
		case SYS_FIBO:
			if(!is_user_vaddr(f->esp+4)) exit(-1);
			f->eax = fibonacci((int)*(uint32_t*)(f->esp+4));
			break;
		case SYS_MAX:
			if(!is_user_vaddr(f->esp+4)) exit(-1);
			f->eax = max_of_four_int((int)*(uint32_t*)(f->esp+4), (int)*(uint32_t*)(f->esp+8), (int)*(uint32_t*)(f->esp+12), (int)*(uint32_t*)(f->esp+16));
			break;
	}
  //thread_exit ();
}
void halt(void){
	shutdown_power_off();
}
void exit(int status){
	find_thread(thread_current()->parent_tid)->child_status = status;
	//let parent know child's status
	
	printf("%s: exit(%d)\n", thread_current()->name, status);
	thread_exit();
}
pid_t exec(const char* cmd_line){
	return process_execute(cmd_line);
}
int wait(pid_t pid){
	return process_wait(pid);
}
int read(int fd, void* buffer, unsigned size){
	if(fd == 0) {
		//call input_getc()
		//prototype : uint8_t input_getc(void)
		//return a char from keyboards
		unsigned i;
		for(i=0;i<size;i++){
			*(uint8_t*)buffer = input_getc();
			if(((char*)buffer)[i] == '\0') break;
		}
		return i;
	}
	return -1;
}
int write(int fd, const void* buffer, unsigned size){
	if(fd==1) {
		putbuf(buffer, size);
		return size;
	}
	return -1;
}
int fibonacci(int n){
	int i;
	int* seq = (int*)malloc(sizeof(int)*n);
	seq[0] = 1;
	seq[1] = 1;
	for(i=2; i<=n; i++){
		seq[i] = seq[i-1]+seq[i-2];
	}
	i = seq[n-1];
	free(seq);
	return i;
}

int max_of_four_int(int n0, int n1, int n2, int n3){
	int max = n0;
	int arr[4] = {n0,n1,n2,n3};
	int i;
	for(i=1; i<4; i++){
		if(max <= arr[i]) max = arr[i];
	}
	return max;
}
