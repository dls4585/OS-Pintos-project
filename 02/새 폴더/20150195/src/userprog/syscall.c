#include "userprog/syscall.h"
#include <stdio.h>
#include <syscall-nr.h>
#include <string.h>
#include "threads/interrupt.h"
#include "threads/thread.h"
#include "threads/vaddr.h"
#include "devices/shutdown.h"
#include "userprog/process.h"
#include "devices/input.h"
#include "threads/malloc.h"
#include "filesys/filesys.h"
#include "filesys/file.h"
#include "threads/synch.h"
struct file
  {
    struct inode *inode;        /* File's inode. */
    off_t pos;                  /* Current position. */
    bool deny_write;            /* Has file_deny_write() been called? */
  };
struct lock fs_lock;
static void syscall_handler (struct intr_frame *);
void
syscall_init (void) 
{
  lock_init(&fs_lock);
  intr_register_int (0x30, 3, INTR_ON, syscall_handler, "syscall");
}

static void
syscall_handler (struct intr_frame *f) 
{
//	printf("%d\n", *(uint32_t*)(f->esp));
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
		case SYS_CREATE:
			if(!is_user_vaddr(f->esp+4)) exit(-1);
			if(!is_user_vaddr(f->esp+8)) exit(-1);
			 f->eax = create((const char *)*(uint32_t *)(f->esp + 4), (unsigned)*(uint32_t *)(f->esp + 8));
			break;
		case SYS_REMOVE:
			if(!is_user_vaddr(f->esp+4)) exit(-1);
			f->eax = remove((const char*)*(uint32_t *)(f->esp + 4));
			break;
		case SYS_OPEN:
			if(!is_user_vaddr(f->esp+4)) exit(-1);
			f->eax = open((const char*)*(uint32_t *)(f->esp + 4));
			break;
		case SYS_FILESIZE:
			if(!is_user_vaddr(f->esp+4)) exit(-1);
			f->eax = filesize((int)*(uint32_t *)(f->esp + 4));
			break;
		case SYS_SEEK:
			if(!is_user_vaddr(f->esp+4)) exit(-1);
			if(!is_user_vaddr(f->esp+8)) exit(-1);
			seek((int)*(uint32_t *)(f->esp + 4), (unsigned)*(uint32_t *)(f->esp + 8));
			break;
		case SYS_CLOSE:
			if(!is_user_vaddr(f->esp+4)) exit(-1);
			close((int)*(uint32_t *)(f->esp + 4));
			break;
		case SYS_TELL:
			if(!is_user_vaddr(f->esp+4)) exit(-1);
			f->eax = tell((int)*(uint32_t *)(f->esp + 4));
			break;
	}
  //thread_exit ();
}
void halt(void){
	shutdown_power_off();
}
void exit(int status){
	thread_current()->exit_status = status;
	printf("%s: exit(%d)\n", thread_current()->name, status);
	for(int i = 3; i < 128; i++)
		if(thread_current()->files[i] != NULL) close(i);
	thread_exit();
}
pid_t exec(const char* cmd_line){
	return process_execute(cmd_line);
}
int wait(pid_t pid){
	return process_wait(pid);
}
int read(int fd, void* buffer, unsigned size){
	if(!is_user_vaddr(buffer)) exit(-1);
	if(!is_user_vaddr(buffer+size)) exit(-1);
	int ret = -1;
	if(fd == 0) {
		//call input_getc()
		//prototype : uint8_t input_getc(void)
		//return a char from keyboards
		unsigned i;
		for(i=0;i<size;i++){
			*(uint8_t*)buffer = input_getc();
			if(((char*)buffer)[i] == '\0') break;
		}
		ret = i;
	} else if (fd >= 3) {
		if(thread_current()->files[fd] == NULL) {
			exit(-1);
		}
		ret = file_read(thread_current()->files[fd], buffer, size);
	}
	return ret;
}
int write(int fd, const void* buffer, unsigned size){
	if(!is_user_vaddr(buffer)) exit(-1);
	if(!is_user_vaddr(buffer+size)) exit(-1);
	lock_acquire(&fs_lock);
	int ret = -1;
	if(fd==1) {
		putbuf(buffer, size);
		ret = size;
	} else if (fd >= 3){
		if(thread_current()->files[fd] == NULL){
			lock_release(&fs_lock);
			exit(-1);
		}
		if(thread_current()->files[fd]->deny_write) file_deny_write(thread_current()->files[fd]);
		ret = file_write(thread_current()->files[fd], buffer, size);
	}
	lock_release(&fs_lock);
	return ret;
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


bool create(const char* filename, unsigned size) {
	if(filename == NULL) exit(-1);
	if(!is_user_vaddr(filename)) exit(-1);
	return filesys_create(filename, size);
}

bool remove(const char* filename) {
	if(filename == NULL) exit(-1);
	if(!is_user_vaddr(filename)) exit(-1);
	return filesys_remove(filename);
}

int open(const char* filename) {
	if(filename == NULL) exit(-1);
	if(!is_user_vaddr(filename)) exit(-1);
	lock_acquire(&fs_lock);
	int i, ret = -1;
	struct file* new_file = filesys_open(filename);
	if(new_file == NULL) ret= -1;
	else {
		if(strcmp(thread_current()->name, filename) == 0) file_deny_write(new_file);
		for (i=3; i< 128; i++) {
			if(thread_current()->files[i] == NULL) {
				thread_current()->files[i] = new_file;
				ret = i;
				break;
			}
		}
	}
	lock_release(&fs_lock);
	return ret;
}

int filesize (int fd) {
	if(thread_current()->files[fd] == NULL) exit(-1);
	return file_length(thread_current()->files[fd]);
}

void seek (int fd, unsigned pos){
	if(thread_current()->files[fd] == NULL) exit(-1);
	file_seek(thread_current()->files[fd], pos);
}

unsigned tell (int fd) {
	if(thread_current()->files[fd] == NULL) exit(-1);
	return file_tell(thread_current()->files[fd]);
}

void close (int fd) {
	if(thread_current()->files[fd] == NULL) exit(-1);
	file_close(thread_current()->files[fd]);
	thread_current()->files[fd] = NULL;
}
