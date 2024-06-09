#include <stdlib.h>
#include <time.h>

void fbt_ssbanger(int nbang) {
    // Initialize the FAT
    fat_init();

    // Seed the random number generator
    srand(time(NULL));

    // Array to keep track of allocated clusters
    int allocated_clusters[TOTAL_CLUSTERS];
    int allocated_count = 0;

    for (int i = 0; i < nbang; i++) {
        if (rand() % 2 == 0) {
            // Allocate operation
            int size = rand() % TOTAL_CLUSTERS + 1;
            int start_cluster = fat_alloc(size);
            if (start_cluster != -1) {
                allocated_clusters[allocated_count++] = start_cluster;
            }
        } else {
            // Free operation
            if (allocated_count > 0) {
                int index = rand() % allocated_count;
                fat_free(allocated_clusters[index]);
                allocated_clusters[index] = allocated_clusters[--allocated_count];
            }
        }
    }

    // Verify the FAT
    fat_verify();
}