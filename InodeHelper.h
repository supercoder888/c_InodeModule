#ifndef INODEHELPER_H_
#define INODEHELPER_H_

#include "InodeFileReader.h"
#include "InodeDefines.h"

#include <string>
#include <fstream>
#include <iostream>
#include <sstream>
#include <cmath>
#include <cstring>
#include <cstdlib>

#include <unistd.h>
#include <sys/types.h>


class InodeHelper
{
    
public:
    InodeHelper();
    ~InodeHelper();
    
    bool setArgument(std::string a_Path);
    void setFileName(std::string a_Name);
    void closeFile(std::string a_Name);
    size_t determineInodeNumber(size_t a_Address);
        
    int extentRecursionRoot(InodeFileReader &a_Reader, size_t a_Address, bool a_IsFile, std::map<size_t, std::string> &a_InodeInformation, std::map<size_t, directoryTree> &a_InodeTree, bool a_Simple);
    
    std::set<size_t> m_FolderInodes;
    
    // Important ext4 parameters set from outside
    std::string m_OutputFolder;
    size_t m_ExtBlockSize;
    size_t m_ExtInodeSize;
    size_t m_ExtBlockCount;
    size_t m_ImgSize;
    size_t m_ExtBlockGroupSize;
    size_t m_ImgOffset;
    size_t m_FlexGroupSize;
    size_t m_64Bit;
    bool m_Sparse;
    size_t m_InodeRatio;
    size_t m_TimeStampMin;
    size_t m_TimeStampMax;
    bool m_TimeStampSet;
    std::vector<unsigned short> m_Permission;
    
    // Important ext4 parameters calculated
    size_t m_NumberOfBlockGroups;
    size_t m_InodeTableOffset;
    size_t m_NumOfInodesPerGroup;
    size_t m_SuperGrowth;

private:
    int extentRecursion(InodeFileReader &a_Reader, size_t a_Address, size_t a_ExpectedDepth, size_t a_MaxDepth, bool a_IsFile, bool a_IsDirHash);
    int readDirectoryEntry(InodeFileReader &a_Reader, bool a_IsDirHash);
    
    void calculateImageParameters();
    void setDefault();
    
    size_t roundToFour(size_t a_Value);
    size_t roundToNext(size_t a_Value, size_t a_Round);
    
    std::map<size_t, directoryTree> m_TempTree;
    std::map<size_t, std::string> m_TempInodeInformation;
    std::vector<ext4Bytes> m_RecoveredInodes;
    
    size_t m_Parent;
    size_t m_TempNum;
    bool m_IsRoot;
    
    unsigned char *m_DataBuffer;
    std::ofstream m_File;
    
    size_t m_Rest;
};

#endif
