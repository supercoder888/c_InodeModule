#ifndef INODECARVER_H_
#define INODECARVER_H_

//#include "tsk/framework/services/TskServices.h"
#include "InodeDefines.h"
#include "InodeFileReader.h"
#include "InodeHelper.h"

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <map>
#include <algorithm>
#include <cstdio>
#include <ctime>
#include <cstring>

class InodeCarver
{
public:
    InodeCarver();
    ~InodeCarver();

    void printInodeStats();
    bool setArgument(std::string a_Path, InodeFileReader &a_Reader);
    void searchInode(size_t a_Size, size_t a_Count, InodeFileReader &a_Reader);
    void writeRegFiles(InodeFileReader &a_Reader, bool a_Simple);
    void gatherInodeInformation(InodeFileReader &a_Reader);
    
private:
    std::string directoryTreeRec(size_t a_InodeNum, size_t a_Depth);
    
    InodeHelper m_Helper;
    
    std::vector<size_t> m_RegFiles;
    std::vector<size_t> m_Directories;
    
    // NOTE: m_InodeInformation turned out to be redundant and should be replaceable by m_InodeTree.second.name entries.
    std::map<size_t, std::string> m_InodeInformation;
    std::map<size_t, directoryTree> m_InodeTree;
    
    // Debug parameters
    size_t m_FoundInodes;
    size_t m_FoundOtherTypes;
    size_t m_FoundPermission;
    size_t m_FoundLink;
    size_t m_FoundTimeStamp;
    size_t m_FoundExtent;
    size_t m_FoundRegFiles;
    size_t m_FoundDupliDir;
    size_t m_FoundDupliReg;
    size_t m_FoundDirectories;
    size_t m_RegNotValid;
    size_t m_RegFailure;
    size_t m_DirFailure;
    size_t m_FoundDeletedReg;
    size_t m_FoundDeletedDir;

    // Evaluation
    std::vector<size_t> permissionVec;
    std::vector<size_t> timeStampVec;
    std::vector<size_t> extentFlagVec;
    std::vector<size_t> extentHeaderVec;
    std::vector<size_t> fileTypeVec;
    std::vector<size_t> linkCountVec;
};

#endif
