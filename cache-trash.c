/* Peter Hornyack
 * Created 2012-05-21
 */

#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include "kp_macros.h"
#include "kp_recovery.h"
#include "ptlcalls.h"

#define DO_PTLSIM

/* Forward declarations: */
void set_process_affinity(int cpu);
void idea1(void);

int main(int argc, char *argv[])
{
	set_process_affinity(0);  //pin to CPU 0...
	idea1();

	return 0;
}

/* Sets this process' affinity to run on only a single CPU. NUM_CPUS
 * must be set before calling this function.
 *
 * I'm not sure what impact this has on a multi-threaded process: will
 * it force all of the process' threads onto a single CPU? It seems
 * like we would have noticed this already if this were true, but
 * who knows.
 */
void set_process_affinity(int cpu)
{
	int ret;
	cpu_set_t mask;

	if (cpu < 0) {
		kp_die("invalid cpu: %d\n", cpu);
	}

	CPU_ZERO(&mask);
	CPU_SET(cpu, &mask);  //add CPU 1 to mask

	ret = sched_setaffinity(0, sizeof(cpu_set_t), &mask);
	if (ret != 0) {
		kp_die("ERROR: sched_setaffinity() returned %d\n", ret);
	}

	kp_print("pinned this process to CPU=0x%X\n", cpu);
}

/* Total cache size (in bytes) is BLOCK_SIZE * SETS * WAYS.
 * 8 MB cache (8388608 bytes): 64 * 8192 * 16
 * 2 MB cache (2097152 bytes): 64 * 4096 *  8*/
//#define CACHE_8MB
#define CACHE_2MB

#ifdef CACHE_8MB
#define BLOCK_SIZE 64
#define SETS 8192
#define WAYS 16
#endif
#ifdef CACHE_2MB
#define BLOCK_SIZE 64
#define SETS 4096
#define WAYS 8
#endif

#define NUM_MEM_REGIONS 2
#define NUM_LOOPS 1

void idea1(void)
{
#ifdef KP_PRINT
	const char *description = "Idea 1: simply write every single byte "
		"in a memory region that is the size of the L3 cache, then write "
		"every single byte again in a _different_ memory region that is "
		"the size of the L3 cache. Alternate back and forth between these "
		"two memory regions several times, so that each memory region "
		"causes the other memory region to be evicted when it is written "
		"in the cache.";
#endif
	kp_print("%s\n", description);

	unsigned int i, j, k;
	size_t mem_region_size = BLOCK_SIZE * SETS * WAYS;
	  //to try: multiply by 2, to make sure alignment, CPU switching issues
	  //  don't impact us?
	unsigned int num_mem_regions = NUM_MEM_REGIONS;
	char initial_byte = 'A';
	char **mem_regions;

	kp_print("Allocating %u memory regions of size %lu bytes each "
			"(%lu KB)\n",
			num_mem_regions, mem_region_size, mem_region_size / 1024);
	mem_regions = malloc(num_mem_regions * sizeof(char *));
	if (!mem_regions) {
		kp_die("malloc(mem_regions) failed\n");
	}
	for (i = 0; i < num_mem_regions; i++) {
		mem_regions[i] = malloc(mem_region_size);
		if (!mem_regions[i]) {
			kp_die("malloc(mem_region_size) failed\n");
		}
		// Don't do this, don't know how it behaves:
		//memset(mem_regions[i], (int)initial_byte, mem_region_size);
	}

	char new_byte = initial_byte + 1;
	unsigned int num_loops = NUM_LOOPS;
#ifndef DO_PTLSIM
	unsigned int print_frequency = 1;
#endif

	kp_print("Trashing cache: block_size=%u B, sets=%u, ways=%u\n",
			(unsigned int)BLOCK_SIZE, (unsigned int)SETS,
			(unsigned int)WAYS);
#ifdef DO_PTLSIM
	kp_print("PTL: calling ptlcall_switch_to_sim(), then running cache-trash "
			"loop\n");
	ptlcall_switch_to_sim();
#endif
	for (i = 0; i < num_loops; i++) {
#ifndef DO_PTLSIM
		if (i % print_frequency == 0) {
			kp_print("Trashing cache: loop %u, new_byte=%c, %u memory "
					"regions\n", i, new_byte, num_mem_regions);
		}
#endif
		for (j = 0; j < num_mem_regions; j++) {
			//for (k = 0; k < mem_region_size; k++) {  //try: k += BLOCK_SIZE?
			for (k = 0; k < mem_region_size; k+= BLOCK_SIZE) {
				/* I printed out the first address that's modified here,
				 * and it's not cache-block-aligned (this byte is not the
				 * first byte of a cache block); its six lowest-order bits
				 * (2^6 = 64 B) were 010000.
				 */
				(mem_regions[j])[k] = new_byte;
				//kp_print("flushing address %p (byte=%c)\n",
				//		mem_regions[j]+k, mem_regions[j][k]);
				//kp_die("test\n");
				flush_range((mem_regions[j])+k, sizeof(new_byte));
			}
		}
		new_byte++;
	}
#ifdef DO_PTLSIM
	ptlcall_single_flush("-stop");
	kp_print("PTL: just called ptlcall_single_flush(\"-stop\")\n");
#endif
	kp_print("Trashed cache for %u loops; final new_byte=%c\n", i, new_byte);
	kp_print("Total number of writes to memory regions: %lu\n",
			//num_loops * num_mem_regions * mem_region_size);
			num_loops * num_mem_regions * mem_region_size / BLOCK_SIZE);

	for (i = 0; i < num_mem_regions; i++) {
		free(mem_regions[i]);
	}
	free(mem_regions);
	kp_print("freed memory regions\n");
}

//Idea 2: like idea 1, but only write one byte per block! (alignment issues?)


