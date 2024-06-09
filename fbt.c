//
// Created by Akatsuki Sky on 6/8/2024.
//
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include "getcmd.c"
#include "ssbanger2.inc"

// Here I use cluster instead of block
#define TOTAL_CLUSTERS 40000 // Total number of clusters(blocks) in the FAT
#define FAT_EOF -1         // End of file marker
#define EMPTY_CLUSTER -2   // Empty cluster marker
#define MAX_FILES 40000      // Maximum number of files
int start_clusters[MAX_FILES]; // Start clusters of the files
int remaining_clusters;    // Remaining clusters(blocks) in the FAT
typedef struct {
    int next_cluster;
} FATEntry;
FATEntry fat[TOTAL_CLUSTERS]; // File Allocation Table (FAT)

void fat_init() {
    for (int i = 0; i < TOTAL_CLUSTERS; i++) {
        fat[i].next_cluster = EMPTY_CLUSTER; // Initialize the FAT
    }
    for (int i = 0; i < MAX_FILES; i++) {
        start_clusters[i] = -1;              // Initialize the start clusters
    }
    remaining_clusters = TOTAL_CLUSTERS;
}

int find_free_cluster() {
    for (int i = 0; i < TOTAL_CLUSTERS; i++) {
        if (fat[i].next_cluster == EMPTY_CLUSTER) { // Find the first empty cluster
            fat[i].next_cluster = i + 1;            // Mark the cluster as used
            return i;
        }
    }
    return -1;
}

int fat_alloc(int size) {
    if (size > remaining_clusters) { // Not enough clusters
        printf("Memory allocation failed : out of memory.\n");
        return -1;
    }
    int start_cluster = find_free_cluster(); // Find the first free cluster

    // Find the first empty position in start_clusters array
    int i = 0;
    while (i < MAX_FILES && start_clusters[i] != -1) {
        i++;
    }

    if (i == MAX_FILES) { // The array is full
        printf("The start_clusters array is full\n");
        return -1;
    }

    // Store the start cluster of the new file in the array
    start_clusters[i] = start_cluster;

    int current_cluster = start_cluster;     // Current cluster
    for (int i = 0; i < size; i++) {         // Allocate clusters for the file
        int next_cluster = find_free_cluster(); // Find the next free cluster

        fat[current_cluster].next_cluster = next_cluster;
        current_cluster = next_cluster;
    }

    fat[current_cluster].next_cluster = FAT_EOF;
    remaining_clusters -= size + 1; // Update the remaining clusters with plus 1 EOF cluster

    return start_cluster;
}

void fat_free(int start_cluster) {
    int current_cluster = start_cluster;
    while (current_cluster != FAT_EOF) {
        int next_cluster = fat[current_cluster].next_cluster;
        fat[current_cluster].next_cluster = EMPTY_CLUSTER;
        current_cluster = next_cluster;
        remaining_clusters++; // Update the remaining clusters
    }
    for(int i = 0; i < MAX_FILES; i++) {
        if (start_clusters[i] == start_cluster) {
            start_clusters[i] = -1; // Mark the start cluster as free
            break;
        }
    }
}
void fat_verify() {
    int allocated_clusters = 0;
    for (int i = 0; i < TOTAL_CLUSTERS; i++) {
        if (fat[i].next_cluster != EMPTY_CLUSTER) {
            allocated_clusters++;
        }
    }
    if (allocated_clusters != (TOTAL_CLUSTERS - remaining_clusters)) {
        printf("FAT integrity has been compromised.\n");
    } else {
        printf("FAT is valid.\n");
    }
}

void fat_display() {
    for (int count = 0; count < TOTAL_CLUSTERS; count++) {
        putchar((fat[count].next_cluster == -2) ? '0' : '1');
        if (count % 64 == 63) {         //every 64 bits one line
            putchar('\n');
        } else if (count % 8 == 7) {    //every 8 bits one space
            putchar(' ');
        }
    }
}

void print_start_clusters() {
    printf("Start clusters: ");
    for (int i = 0; i < MAX_FILES; i++) {
        if (start_clusters[i] != -1) { // Print the start clusters which are not empty
            printf("%d ", start_clusters[i]);
        }
    }
    printf("\n");
}

void fat_dump(int start_cluster) {
    int* cluster_indices = NULL;
    int count = 0;
    int current_cluster = start_cluster;
    while (current_cluster != FAT_EOF) {
        // Reallocate memory for the array
        cluster_indices = realloc(cluster_indices, (count + 1) * sizeof(int));
        if (cluster_indices == NULL) {
            fprintf(stderr, "Memory allocation failed.\n");
            return;
        }
        // Add the current cluster index to the array
        cluster_indices[count] = current_cluster;
        count++;
        // Move to the next cluster in the chain
        if (fat[current_cluster].next_cluster == EMPTY_CLUSTER) {
            break;
        }
        current_cluster = fat[current_cluster].next_cluster;
    }

    // Print the array
    printf("Cluster indices: ");
    for (int i = 0; i < count; i++) {
        printf("%d ", cluster_indices[i]);
    }
    printf("\n");
    // Free the memory allocated for the array
    free(cluster_indices);
}
void fbt_interactive() {
    int bn;
    char cmd;
    int param1, param2;
    int ic;

    for (;;) {
        fputs("fbt>", stdout); fflush(stdout);
        switch((ic = getcmd(&cmd, &param1, &param2))) { // get command
            case 0: // EOF
                goto out;

            case 1: // Command only
                if (cmd == 'd')
                    fat_display();
                else if (cmd == 'v')
                    fat_verify();
                else if (cmd == 'p')
                    print_start_clusters();
                break;

            case 2: // Command and a parameter
                if (cmd == 'a') {
                    bn = fat_alloc(param1);
                    printf("A %d %d\n", bn, param1);
                }
                else if (cmd == 'b') {
                    ssbanger(param1, fat_alloc, (fp_free_t) fat_free, NULL, NULL, NULL);
                }
                else if (cmd == 'd') {
                    fat_dump(param1);
                }
                else if (cmd == 'f') {
                    fat_free(param1);
                    printf("F %d\n", param1);
                }
                else{
                    fprintf(stderr, "bad command '%c' "
                                    "or invalid parameter(s)\n", cmd);
                }
                break;

            case 3: // Command and two parameters
                if (cmd == 'b') {
                   // ssbanger_mt(param1, param2, fat_alloc, fat_free, NULL, NULL, NULL);
                }
                else {
                    fprintf(stderr, "bad command '%c' "
                                    "or invalid parameter(s)\n", cmd);
                }
                break;

            case -1: // Error
                break;
        }
    }

    out:;
}
int main(void) {

#if 0
    fat_init();
    fat_alloc(20);
    fat_display();
    fat_alloc(30);
    fat_display();
    fat_free(0);
    fat_dump(5);
    print_start_clusters();
    return 0;
#else
    fat_init();
    fbt_interactive();
    return 0;
#endif

    return 0;
}

