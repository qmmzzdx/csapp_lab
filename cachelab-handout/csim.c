#include <stdio.h>
#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <unistd.h>
#include "cachelab.h"
#define MAXLINE 512

typedef struct
{
    int valid;
    int tag;
    int timestamp;
} CacheLine, *CacheGroup, **Cache;

static Cache cache;
static int s, S, E, b;
static int hit, miss, evict;
static char file_path[MAXLINE];

void get_input(int argc, char *argv[]);
void initialize_cache();
void lru_update_timestamp();
void access_cache();
void simulator();
void free_cache();

int main(int argc, char *argv[])
{
    get_input(argc, argv);
    initialize_cache();
    simulator();
    free_cache();
    printSummary(hit, miss, evict);

    return 0;
}

void get_input(int argc, char *argv[])
{
    int opt;

    while ((opt = getopt(argc, argv, "s:E:b:t:")) != -1)
    {
        switch (opt)
        {
            case 's':
            {
                s = atoi(optarg);
                S = 1 << s;
                break;
            }
            case 'E':
            {
                E = atoi(optarg);
                break;
            }
            case 'b':
            {
                b = atoi(optarg);
                break;
            }
            case 't':
            {
                strncpy(file_path, optarg, MAXLINE - 1);
                file_path[MAXLINE - 1] = '\0';
                break;
            }
        }
    }
}

void initialize_cache()
{
    int i;

    cache = (Cache)malloc(sizeof(CacheGroup) * S);
    assert(cache);
    for (i = 0; i < S; i++)
    {
        cache[i] = (CacheGroup)malloc(sizeof(CacheLine) * E);
        assert(cache[i]);
        memset(cache[i], 0, sizeof(CacheLine) * E);
    }
}

void lru_update_timestamp()
{
    int i, j;

    for (i = 0; i < S; i++)
    {
        for (j = 0; j < E; j++)
        {
            if (cache[i][j].valid)
            {
                ++cache[i][j].timestamp;
            }
        }
    }
}

void access_cache(uint64_t address)
{
    int i;
    int evict_index = 0;
    int max_timestamp = 0;
    int tag = address >> (s + b);
    uint64_t mask = (1ULL << s) - 1;
    CacheGroup current_group = cache[(address >> b) & mask];

    for (i = 0; i < E; i++)
    {
        if (current_group[i].valid && current_group[i].tag == tag)
        {
            ++hit;
            current_group[i].timestamp = 0;
            return;
        }
    }
    ++miss;
    for (i = 0; i < E; i++)
    {
        if (current_group[i].valid == 0)
        {
            current_group[i].valid = 1;
            current_group[i].tag = tag;
            current_group[i].timestamp = 0;
            return;
        }
    }
    ++evict;
    for (i = 0; i < E; i++)
    {
        if (max_timestamp < current_group[i].timestamp)
        {
            max_timestamp = current_group[i].timestamp;
            evict_index = i;
        }
    }
    current_group[evict_index].tag = tag;
    current_group[evict_index].timestamp = 0;
}

void simulator()
{
    FILE *fp = fopen(file_path, "r");
    assert(fp);

    char op[2];
    uint64_t address;
    int size;
    while (fscanf(fp, "%s %lx,%d", op, &address, &size) > 0)
    {
        switch (op[0])
        {
            case 'M':
                access_cache(address);
            case 'L':
            case 'S':
                access_cache(address);
                break;
        }
        lru_update_timestamp();
    }
    fclose(fp);
}

void free_cache()
{
    int i;

    for (i = 0; i < S; i++)
    {
        free(cache[i]);
    }
    free(cache);
}
