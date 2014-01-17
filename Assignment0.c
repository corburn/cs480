#include <stdio.h>
int main(int argc, char **argv) {
	int foo = 42;
	int *p_foo = &foo;
	// Print the value of foo
	printf("foo: %d\n", foo);
	printf("p_foo: %d\n", *p_foo);
	// Print the memory address of foo
	printf("foo: %p\n", &foo);
	printf("p_foo: %p\n", p_foo);
	// Update foo via the pointer
	*p_foo = 24;
	// Show foo has changed
	printf("foo: %d\n", foo);
	// Show that the memory address hasn't changed
	printf("foo: %p\n", &foo);
	printf("p_foo: %p\n", p_foo);

	char s1[80]="Hello";	
	char s2[80]="Hello";	
	// s1 and s2 are allocated in separate memory locations
	if(s1==s2) {
		printf("words are the same\n");	
	} else {
		printf("words are different\n");
	}
	char *s3="good-bye";	
	char *s4="good-bye";	
	// The compiler realizes "good-bye" is the same data and allocates it to the same memory location
	// s3 and s4 contain the address of that memory location
	if(s3==s4){
		printf("words are the same\n");	
	} else {
		printf("words are different\n");
	}	
	// s3 and s4 are not located in the same memory location
	if(&s3==&s4){
		printf("words are the same\n");	
	} else {
		printf("words are different\n");	
	}	
	printf("===============\n %p %p", *s3, s4);
}
