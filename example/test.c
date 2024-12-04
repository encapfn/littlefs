#include "../lfs.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

// variables used by the filesystem
lfs_t lfs;
lfs_file_t file;

#define BLOCK_SIZE 4096
#define BLOCK_COUNT 128
#define TOTAL_SIZE 524288 // BLOCK_COUNT * BLOCK_SIZE
uint8_t files[TOTAL_SIZE] = {};

int read(const struct lfs_config * cfg, lfs_block_t block, lfs_off_t off, void * data_out, lfs_size_t siz)
{
    size_t addr = ((block * BLOCK_SIZE) + off);
    // printf("Read Addr: %lu\n", addr);
    // printf("Read Block: %lu\n", block);
    // printf("Read Off: %lu\n", off);
    if (addr + siz > TOTAL_SIZE)
    {
        return -1;
    }

    uint8_t * data_loc = files + addr;
    for (size_t i = 0; i < siz; i++)
    {
        *(((uint8_t*)data_out) + i) = *(data_loc + i);
    }

    return 0;
}

int prog(const struct lfs_config * cfg, lfs_block_t block, lfs_off_t off, const void * data, lfs_size_t siz)
{
    size_t addr = ((block * BLOCK_SIZE) + off);
    if (addr + siz > TOTAL_SIZE)
    {
        return -1;
    }

    uint8_t * data_loc = files + addr;
    for (size_t i = 0; i < siz; i++)
    {
        *(data_loc + i) = *(((uint8_t*)data) + i);
    }

    return 0;
}

int erase(const struct lfs_config * cfg, lfs_block_t block)
{
    size_t addr = (block * BLOCK_SIZE);
    uint8_t * data_loc = files + addr;
    for (size_t i = 0; i < BLOCK_SIZE; i++)
    {
        *(data_loc + i) = 0;
    }
    return 0;
}

int sync(const struct lfs_config * cfg)
{
    
    return 0;
}

// configuration of the filesystem is provided by this struct
const struct lfs_config cfg = {
    // block device operations
    .read  = read,
    .prog  = prog,
    .erase = erase,
    .sync  = sync,

    // block device configuration
    .read_size = 1,
    .prog_size = 1,
    .block_size = BLOCK_SIZE,
    .block_count = BLOCK_COUNT,
    .cache_size = 16,
    .lookahead_size = 16,
    .block_cycles = 500,
};


FILE *fptr;
void init()
{
    // Create a file
    fptr = fopen("data.txt", "r");
    if (fptr == NULL)
    {
        lfs_format(&lfs, &cfg);
        return;
    }
    fread(files, 1, TOTAL_SIZE, fptr);
    fclose(fptr);
}

void deinit()
{
    fptr = fopen("data.txt", "w");
    fwrite(files, 1, TOTAL_SIZE, fptr);
    fclose(fptr);
}

// entry point
int main(void) {
    init();

    // mount the filesystem
    int err = lfs_mount(&lfs, &cfg);

    // read current count
    uint32_t boot_count = 0;
    lfs_file_open(&lfs, &file, "boot_count", LFS_O_RDWR | LFS_O_CREAT);
    lfs_file_read(&lfs, &file, &boot_count, sizeof(boot_count));

    // update boot count
    boot_count += 2;
    lfs_file_rewind(&lfs, &file);
    lfs_file_write(&lfs, &file, &boot_count, sizeof(boot_count));

    // remember the storage is not updated until the file is closed successfully
    lfs_file_close(&lfs, &file);

    // release any resources we were using
    lfs_unmount(&lfs);

    // print the boot count
    printf("boot_count: %d\n", boot_count);

    deinit();
}