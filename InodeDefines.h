#ifndef INODEDEFINES_H_
#define INODEDEFINES_H_

#define BLOCKSIZE       134217728
#define MAXTREEDEPTH    50

#include <string>

typedef struct ext4Extent
{
    unsigned int firstFileBlock;
    unsigned short fileBlockLen;
    unsigned long long blockNum;
} ext4Extent;

typedef struct ext4Bytes
{
    size_t size;
    char a[60];
} ext4Bytes;

typedef struct directoryTree
{
    size_t inodeParent;
    bool complete;
    std::string name;
} directoryTree;

#endif
