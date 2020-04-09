#ifndef INODEFILEREADER_H_
#define INODEFILEREADER_H_

#include "tsk/framework/services/TskServices.h"
#include "InodeDefines.h"

#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <future>

class InodeFileReader
{
public:
    InodeFileReader();
    ~InodeFileReader();
    
    void initialize(size_t a_BlockSize, size_t a_InodeSize, size_t a_ImgOffset, size_t a_ImgSize);
    size_t getRealImgSize();
    std::string getOutFolder();

    int readNewBlock();
    int getInode(size_t a_Address);
    int getDirectory(size_t a_Address);
    int getData(size_t a_Address);
    void reset();

    size_t getBlockSize();

    char *m_CurrBuffer;
    char *m_DataBuffer;
    char *m_InodeBuffer;
    char *m_DirectoryBuffer;

private:
    TskServices &m_Service;
    TskImageFile &m_Image;
    std::ifstream m_File;

    size_t m_CurrAddress;
    size_t m_BlockSize;
    size_t m_ExtBlockSize;
    size_t m_ExtInodeSize;
    size_t m_ImgOffset;
    size_t m_ImgSize;
    std::string m_OutPath;
    
    char *m_LoadBuffer;
    size_t m_Count;
    size_t m_Loaded;
    
    std::future<int> m_threadSync;
    std::mutex m_localMtx;
};

#endif
