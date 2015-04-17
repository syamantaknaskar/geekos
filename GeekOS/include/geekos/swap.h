#include <libc/disk.h>

DiskController H_Disk;

typedef struct
{
	int base_addr;
	int size;
	int first_free_addr;
	int unsigned s_numPages;
	struct Page_List s_freeList;
} Swap;

Swap SwapSpace;