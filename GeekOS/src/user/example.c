#include <fileio.h>
#include <conio.h>
#include <libc/event.h>

int main(int argc, char **argv) 
{
    char buf[1000];
	//int op = Read(1, buf, 1024);
	Print("\nHi!, Rajesh\n");
	int op = ReadBlock("pass.txt", buf, 1024, 1);

	Print("\nDone\n");
	return 1;
} 