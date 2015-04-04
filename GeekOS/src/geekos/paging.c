/*
 * Paging (virtual memory) support
 * Copyright (c) 2001,2003,2004 David H. Hovemeyer <daveho@cs.umd.edu>
 * Copyright (c) 2003,2013,2014 Jeffrey K. Hollingsworth <hollings@cs.umd.edu>
 *
 * All rights reserved.
 *
 * This code may not be resdistributed without the permission of the copyright holders.
 * Any student solutions using any of this code base constitute derviced work and may
 * not be redistributed in any form.  This includes (but is not limited to) posting on
 * public forums or web sites, providing copies to (past, present, or future) students
 * enrolled in similar operating systems courses the University of Maryland's CMSC412 course.
 *
 * $Revision: 1.56 $
 * 
 */

#include <geekos/string.h>
#include <geekos/int.h>
#include <geekos/idt.h>
#include <geekos/kthread.h>
#include <geekos/kassert.h>
#include <geekos/screen.h>
#include <geekos/mem.h>
#include <geekos/malloc.h>
#include <geekos/gdt.h>
#include <geekos/segment.h>
#include <geekos/user.h>
#include <geekos/vfs.h>
#include <geekos/crc32.h>
#include <geekos/paging.h>
#include <geekos/errno.h>
#include <geekos/projects.h>
#include <geekos/smp.h>
#include <geekos/swap.h>
#include <libc/mmap.h>

/* ----------------------------------------------------------------------
 * Public data
 * ---------------------------------------------------------------------- */

/* ----------------------------------------------------------------------
 * Private functions/data
 * ---------------------------------------------------------------------- */

#define SECTORS_PER_PAGE (PAGE_SIZE / SECTOR_SIZE)

/*
 * flag to indicate if debugging paging code
 */
int debugFaults = 0;
#define Debug(args...) if (debugFaults) Print(args)


const pde_t *Kernel_Page_Dir(void) {
    pde_t *pg_dir;
    

}



/*
 * Print diagnostic information for a page fault.
 */
static void Print_Fault_Info(uint_t address, faultcode_t faultCode) {
    extern uint_t g_freePageCount;

    Print("Pid %d: ", CURRENT_THREAD->pid);
    Print("\n Page Fault received, at address %p (%d pages free)\n",
          (void *)address, g_freePageCount);
    if (faultCode.protectionViolation)
        Print("   Protection Violation, ");
    else
        Print("   Non-present page, ");
    if (faultCode.writeFault)
        Print("Write Fault, ");
    else
        Print("Read Fault, ");
    if (faultCode.userModeFault)
        Print("in User Mode\n");
    else
        Print("in Supervisor Mode\n");
}


/*
 * Handler for page faults.
 * You should call the Install_Interrupt_Handler() function to
 * register this function as the handler for interrupt 14.
 */
/*static*/ void Page_Fault_Handler(struct Interrupt_State *state) {
    ulong_t address;
    faultcode_t faultCode;

    KASSERT(!Interrupts_Enabled());

    /* Get the address that caused the page fault */
    address = Get_Page_Fault_Address();
    Debug("Page fault @%lx\n", address);

    if (address < 0xfec01000 && address > 0xf0000000) {
        KASSERT0(0, "page fault address in APIC/IOAPIC range\n");
    }

    /* Get the fault code */
    faultCode = *((faultcode_t *) & (state->errorCode));

    /* rest of your handling code here */
    TODO_P(PROJECT_VIRTUAL_MEMORY_B, "handle page faults");

    TODO_P(PROJECT_MMAP, "handle mmap'd page faults");


  error:
    Print("Unexpected Page Fault received\n");
    Print_Fault_Info(address, faultCode);
    Dump_Interrupt_State(state);
    /* user faults just kill the process */
    if (!faultCode.userModeFault)
        KASSERT0(0, "unhandled kernel-mode page fault.");

    /* For now, just kill the thread/process. */
    Exit(-1);
}

void Idenity_Map_Page(pde_t * currentPageDir, unsigned int address, int flags) {
}

/* ----------------------------------------------------------------------
 * Public functions
 * ---------------------------------------------------------------------- */


/*
 * Initialize virtual memory by building page tables
 * for the kernel and physical memory.
 */
void Init_VM(struct Boot_Info *bootInfo) {
    /*
     * Hints:
     * - Build kernel page directory and page tables
     * - Call Enable_Paging() with the kernel page directory
     * - Install an interrupt handler for interrupt 14,
     *   page fault
     * - Do not map a page at address 0; this will help trap
     *   null pointer references
     */
    if (!bootInfo->memSizeKB) {
        /* look through memsize for a region starting at 0x100000 */
        int i;

        for (i = 0; i < bootInfo->numMemRegions; i++) {
            if ((bootInfo->memRegions[i].baseAddr_low == 0x100000) &&
                (bootInfo->memRegions[i].type == 1)) {
                bootInfo->memSizeKB =
                    bootInfo->memRegions[i].length_low / 1024;
            }
        }
        bootInfo->memSizeKB += 0x1000;
    }

    ulong_t numPages = bootInfo->memSizeKB >> 2;
    ulong_t endOfMem = numPages * PAGE_SIZE;
    unsigned numPageListBytes = sizeof(struct Page) * numPages;
    ulong_t pageListAddr;
    ulong_t kernEnd;
    ulong_t pageListEnd;

    KASSERT(bootInfo->memSizeKB > 0);
    /*getting kernel page dir and calling enable paging on kernel
    page dir */
    pde_t* Page_Dir = Kernel_Page_Dir();
    Enable_Paging(&Page_Dir);
    /*
     * Before we do anything, switch from setup.asm's temporary GDT
     * to the kernel's permanent GDT.
     */
    Init_GDT(0);

    /*
     * We'll put the list of Page objects right after the end
     * of the kernel, and mark it as "kernel".  This will bootstrap
     * us sufficiently that we can start allocating pages and
     * keeping track of them.
     */
    pageListAddr = (HIGHMEM_START + KERNEL_HEAP_SIZE);
    if (pageListAddr >= endOfMem) {
        Print
            ("there is no memory for the page list.  physical memory is too small for the heap %u, bytes after %u. endOfMem=%lu .",
             KERNEL_HEAP_SIZE, HIGHMEM_START, endOfMem);
        KASSERT0(pageListAddr < endOfMem,
                 "there is no memory for the page list.  physical memory is too small for the heap.");
    }
    g_pageList = (struct Page *)pageListAddr;
    pageListEnd = Round_Up_To_Page(pageListAddr + numPageListBytes);

    // clear page list
    memset((void *)g_pageList, '\0', (pageListEnd - (ulong_t) g_pageList));

    kernEnd = Round_Up_To_Page((int)&end);
    s_numPages = numPages;

    /*
     * The initial kernel thread and its stack are placed
     * just beyond the ISA hole.
     */
    KASSERT(ISA_HOLE_END == KERN_THREAD_OBJ);
    KASSERT(KERN_STACK == KERN_THREAD_OBJ + PAGE_SIZE);

    /*
     * Memory looks like this:
     * 0 - start: available (might want to preserve BIOS data area)
     * start - end: kernel
     * end - ISA_HOLE_START: available
     * ISA_HOLE_START - ISA_HOLE_END: used by hardware (and ROM BIOS?)
     * ISA_HOLE_END - HIGHMEM_START: used by initial kernel thread
     * HIGHMEM_START - end of memory: available
     *    (the kernel heap is located at HIGHMEM_START; any unused memory
     *    beyond that is added to the freelist)
     */

    Add_Page_Range(0, PAGE_SIZE, PAGE_UNUSED);
    Add_Page_Range(PAGE_SIZE, KERNEL_START_ADDR, PAGE_AVAIL);
    Add_Page_Range(KERNEL_START_ADDR, kernEnd, PAGE_KERN);
    Add_Page_Range(kernEnd, ISA_HOLE_START, PAGE_AVAIL);
    Add_Page_Range(ISA_HOLE_START, ISA_HOLE_END, PAGE_HW);
    Add_Page_Range(ISA_HOLE_END, HIGHMEM_START, PAGE_ALLOCATED);
    Add_Page_Range(HIGHMEM_START, HIGHMEM_START + KERNEL_HEAP_SIZE,
                   PAGE_HEAP);
    Add_Page_Range(pageListAddr, pageListEnd, PAGE_KERN);
    if (pageListEnd > endOfMem) {
        KASSERT0(pageListEnd < endOfMem,
                 "there is no memory after the page list.  physical memory is too small.");
        /* this would fail at the next line (add_page_range), so this kassert just fails early. */
    }
    Add_Page_Range(pageListEnd, endOfMem, PAGE_AVAIL);

    /* Initialize the kernel heap */
    Init_Heap(HIGHMEM_START, KERNEL_HEAP_SIZE);

    Print
        ("%uKB memory detected, %u pages in freelist, %d bytes in kernel heap\n",
         bootInfo->memSizeKB, g_freePageCount, KERNEL_HEAP_SIZE);
    TODO_P(PROJECT_VIRTUAL_MEMORY_A,
           "Build initial kernel page directory and page tables");
}

void Init_Secondary_VM() {
    TODO_P(PROJECT_VIRTUAL_MEMORY_A, "enable paging on secondary cores");
}

/**
 * Initialize paging file data structures.
 * All filesystems should be mounted before this function
 * is called, to ensure that the paging file is available.
 */
void Init_Paging(void) {
    TODO_P(PROJECT_VIRTUAL_MEMORY_B,
           "Initialize paging file data structures");
}

/**
 * Find a free bit of disk on the paging file for this page.
 * Interrupts must be disabled.
 * @return index of free page sized chunk of disk space in
 *   the paging file, or -1 if the paging file is full
 */
int Find_Space_On_Paging_File(void) {
    KASSERT(!Interrupts_Enabled());
    TODO_P(PROJECT_VIRTUAL_MEMORY_B, "Find free page in paging file");
    return EUNSUPPORTED;
}

/**
 * Free a page-sized chunk of disk space in the paging file.
 * Interrupts must be disabled.
 * @param pagefileIndex index of the chunk of disk space
 */
void Free_Space_On_Paging_File(int pagefileIndex) {
    KASSERT(!Interrupts_Enabled());
    TODO_P(PROJECT_VIRTUAL_MEMORY_B, "Free page in paging file");
}

/**
 * Write the contents of given page to the indicated block
 * of space in the paging file.
 * @param paddr a pointer to the physical memory of the page
 * @param vaddr virtual address where page is mapped in user memory
 * @param pagefileIndex the index of the page sized chunk of space
 *   in the paging file
 */
void Write_To_Paging_File(void *paddr, ulong_t vaddr, int pagefileIndex) {
    struct Page *page = Get_Page((ulong_t) paddr);
    KASSERT(!(page->flags & PAGE_PAGEABLE));    /* Page must be locked! */
    TODO_P(PROJECT_VIRTUAL_MEMORY_B, "Write page data to paging file");
}

/**
 * Read the contents of the indicated block
 * of space in the paging file into the given page.
 * @param paddr a pointer to the physical memory of the page
 * @param vaddr virtual address where page will be re-mapped in
 *   user memory
 * @param pagefileIndex the index of the page sized chunk of space
 *   in the paging file
 */
void Read_From_Paging_File(void *paddr, ulong_t vaddr, int pagefileIndex) {
    struct Page *page = Get_Page((ulong_t) paddr);
    KASSERT(!(page->flags & PAGE_PAGEABLE));    /* Page must be locked! */
    TODO_P(PROJECT_VIRTUAL_MEMORY_B, "Read page data from paging file");
}


void *Mmap_Impl(void *ptr, unsigned int length, int prot, int flags, int fd) {
    TODO_P(PROJECT_MMAP, "Mmap setup mapping");
    return NULL;
}

bool Is_Mmaped_Page(struct User_Context * context, ulong_t vaddr) {
    TODO_P(PROJECT_MMAP,
           "is this passed vaddr an mmap'd page in the passed user context");
    return false;
}

void Write_Out_Mmaped_Page(struct User_Context *context, ulong_t vaddr) {
    TODO_P(PROJECT_MMAP, "Mmap write back dirty mmap'd page");
}

int Munmap_Impl(ulong_t ptr) {
    TODO_P(PROJECT_MMAP, "unmapp the pages");
}
