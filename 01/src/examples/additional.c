#include <stdio.h>
#include <stdlib.h>
#include <syscall.h>

int main(int argc, char* argv[]){
	int n0,n1,n2,n3, ret_fibo, ret_max;
	if(argc != 5) {
		printf("usage: additional [num1] [num2] [num3] [num4]\n");
		exit(-1);
	}
	n0 = atoi(argv[1]); n1 = atoi(argv[2]);
	n2 = atoi(argv[3]); n3 = atoi(argv[4]);

	ret_fibo = fibonacci(n0);
	ret_max = max_of_four_int(n0, n1, n2, n3);
	printf("%d %d\n", ret_fibo, ret_max);
	return 0;
}
