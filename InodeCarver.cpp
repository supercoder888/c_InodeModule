#include "include/InodeCarver.h"

/* Constructor
 */
InodeCarver::InodeCarver()
{
    // Permission set
/*    
    m_Permission.push_back(0x124);                          // r--r--r--
    m_Permission.push_back(0x1a4);                          // rw-r--r--
    m_Permission.push_back(0x1b4);                          // rw-rw-r--
    m_Permission.push_back(0x1ed);                          // rwxr-xr-x
    m_Permission.push_back(0x1fd);                          // rwxrwxr-x
    m_Permission.push_back(0x1c0);                          // rwx------
    m_Permission.push_back(0x180);                          // rw-------
    m_Permission.push_back(0x1e8);                          // rwxr-x---
    m_Permission.push_back(0x1f8);                          // rwxrwx---
    */
   
    // Debug values
    m_FoundInodes = 0;
    m_FoundOtherTypes = 0;
    m_FoundPermission = 0;
    m_FoundLink = 0;
    m_FoundTimeStamp = 0;
    m_FoundExtent = 0;
    m_FoundRegFiles = 0;
    m_FoundDirectories = 0;
    m_FoundDupliDir = 0;
    m_FoundDupliReg = 0;
    m_RegNotValid = 0;
    m_FoundDeletedReg = 0;
    m_FoundDeletedDir = 0;
    m_RegFailure = 0;
    m_DirFailure = 0;
}

/* Destructor
 */
InodeCarver::~InodeCarver()
{}

/* Public function to return the number of found inodes.
 */
void InodeCarver::printInodeStats()
{
    std::cout << "\tInode Stats:" << std::endl;
    std::cout << "\tPermission Hits: " << m_FoundPermission << std::endl;
    std::cout << "\tLink Count Hits: " << m_FoundLink << std::endl;
    std::cout << "\tTimeStamp Hits: " << m_FoundTimeStamp << std::endl;
    std::cout << "\tExtent Hits: " << m_FoundExtent << std::endl;
    std::cout << "\tAll found inodes: " << m_FoundInodes << std::endl;
    std::cout << "\tInodes of regular files: " << m_FoundRegFiles << " with duplicates: " << m_FoundDupliReg << std::endl;
//    std::cout << "\tRegFile Vector: " << m_RegFiles.size() << std::endl;
    std::cout << "\tInodes of directories: " << m_FoundDirectories << " with duplicates: " << m_FoundDupliDir << std::endl;
//    std::cout << "\tDir Vector: " << m_Directories.size() << std::endl;
    std::cout << "\tOther Files: " << m_FoundOtherTypes << std::endl;
    std::cout << "\tDeleted Files Reg (inode = 0): " << m_FoundDeletedReg << std::endl;
    std::cout << "\tDeleted Files Dir (inode = 0): " << m_FoundDeletedDir << std::endl;
    std::cout << "\tInvalid Reg: " << m_RegNotValid << std::endl;
    std::cout << "\tFailure Reg: " << m_RegFailure << " Failure Dir: " << m_DirFailure << std::endl;
    
    std::cout << std::endl;
    std::cout << "**~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~**" << std::endl;
    std::cout << std::endl;
}

/* Public function to set the argument with the path to the config file.
 */
bool InodeCarver::setArgument(std::string a_Path, InodeFileReader &a_Reader)
{
    size_t imgSize = a_Reader.getRealImgSize();
    std::string out = a_Reader.getOutFolder();

    m_Helper.m_ImgSize = imgSize;
    m_Helper.m_OutputFolder = out;
    
    bool check = m_Helper.setArgument(a_Path);
    if (check == false)
    {
        return false;
    }
    
    a_Reader.initialize(m_Helper.m_ExtBlockSize, m_Helper.m_ExtInodeSize, m_Helper.m_ImgOffset, m_Helper.m_ImgSize);
    return true;
}

/* Public function to search for an Inode in a data block.
 */
void InodeCarver::searchInode(size_t a_Size, size_t a_Count, InodeFileReader &a_Reader)
{
    size_t n0 = 0;
    size_t n1 = 0;
    size_t n2 = 0;
    size_t n3 = 0;
    size_t n4 = 0;
    size_t n5 = 0;
    size_t n6 = 0;
    size_t n7 = 0;

    for (size_t i = 0; i < a_Size-m_Helper.m_ExtInodeSize; i += 1)
    {
        unsigned short lowerFileMode = static_cast<unsigned short>((unsigned char)a_Reader.m_CurrBuffer[i]);
        unsigned short upperFileMode = static_cast<unsigned short>((unsigned char)a_Reader.m_CurrBuffer[i+1]);

        unsigned short bit_9 = upperFileMode & 0x01;
        bit_9 = bit_9 << 8;
        unsigned short permission = bit_9 | lowerFileMode;
        unsigned short type = upperFileMode & 0xF0;
        type = type << 8;

        if (m_Helper.m_Permission.size() > 0)
        {
            bool b = true;
            for (auto it : m_Helper.m_Permission)
            {
                if (it == permission)
                {
                    b = false;
                    break;
                }
            }
            if (b == true)
                continue;
        }
        ++n0;
        
        unsigned short linkCount = (unsigned char)a_Reader.m_CurrBuffer[i+26] | (unsigned char)a_Reader.m_CurrBuffer[i+27] << 8;
        if (linkCount == 0)
        {
            continue;
        }

        ++n1;
        
        if (m_Helper.m_TimeStampSet == true)
        {
            unsigned int today = m_Helper.m_TimeStampMax;
            unsigned int minTS = m_Helper.m_TimeStampMin;
            unsigned int ctime = (unsigned char)a_Reader.m_CurrBuffer[i+12] | (unsigned char)a_Reader.m_CurrBuffer[i+13] << 8 | (unsigned char)a_Reader.m_CurrBuffer[i+14] << 16 | (unsigned char)a_Reader.m_CurrBuffer[i+15] << 24;
            unsigned int mtime = (unsigned char)a_Reader.m_CurrBuffer[i+16] | (unsigned char)a_Reader.m_CurrBuffer[i+17] << 8 | (unsigned char)a_Reader.m_CurrBuffer[i+18] << 16 | (unsigned char)a_Reader.m_CurrBuffer[i+19] << 24;
            unsigned int dtime = (unsigned char)a_Reader.m_CurrBuffer[i+20] | (unsigned char)a_Reader.m_CurrBuffer[i+21] << 8 | (unsigned char)a_Reader.m_CurrBuffer[i+22] << 16 | (unsigned char)a_Reader.m_CurrBuffer[i+23] << 24;

            if (mtime <= ctime && (dtime == 0 || (dtime >= ctime && dtime >= mtime)))
            {
                if (mtime > minTS && mtime < today && ctime > minTS && ctime < today && dtime < today)
                {}
                else
                {
                    continue;
                }
            }
            else
            {
                continue;
            }
        }
        
        ++n2;
        
        unsigned int flag = (unsigned char)a_Reader.m_CurrBuffer[i+32] | (unsigned char)a_Reader.m_CurrBuffer[i+33] << 8 | (unsigned char)a_Reader.m_CurrBuffer[i+34] << 16 | (unsigned char)a_Reader.m_CurrBuffer[i+35] << 24;
        unsigned int extent = flag & 0x80000;
        
        // If extents are used
        if (extent != 0)
        {
            ++n3;
            unsigned short magic = (unsigned char)a_Reader.m_CurrBuffer[i+40] | (unsigned char)a_Reader.m_CurrBuffer[i+41] << 8;

            // Check for extent tree header
            if (magic == 0xF30A)
            {
                ++n4;
                size_t tempAddress = ((BLOCKSIZE-m_Helper.m_ExtInodeSize)*a_Count)+i;
                if (type == 0x8000)
                {
                    ++n5;
                    m_RegFiles.push_back(tempAddress);
                }
                else if (type == 0x4000)
                {
                    ++n6;
                    m_Directories.push_back(tempAddress);
                }
                else if (type == 0x1000 || type == 0x2000 || type == 0x6000 || type == 0xA000 || type == 0xC000)
                {
                    ++n7;
                }
            }
            // Extent with no magic value...should not happen
        }
        else
        {
            // NOTE No extents. But maybe something else
            // Other approach can be used here maybe...
        }
    }

    m_FoundPermission += n0;
    m_FoundLink += n1;
    m_FoundTimeStamp += n2;
    m_FoundExtent += n3;
    m_FoundInodes += n4;
    m_FoundRegFiles += n5;
    m_FoundDirectories += n6;
    m_FoundOtherTypes += n7;
}

/* Public function to extract the found inodes of regular files.
 */
void InodeCarver::writeRegFiles(InodeFileReader &a_Reader, bool a_Simple)
{
    size_t notNamed = 0;
    for (size_t i = 0; i < m_RegFiles.size(); ++i)
    {
        std::stringstream ss;
        ss << m_RegFiles.at(i);
        std::string str = ss.str();
        std::string name = m_Helper.m_OutputFolder;
        if (a_Simple == true)
        {
            name += str + ".bin";
        }
        else
        {
            // get info of directory-structure
            size_t num = m_Helper.determineInodeNumber(m_RegFiles.at(i));
            
            // Deleted or Journal
            if (num == 0)
            {
                ++m_FoundDeletedReg;
                continue;
            }
            else if (num < 11)                              // Special Inode
            {
                ++m_RegNotValid;
                continue;
            }
            
            if (m_InodeInformation.find(num) != m_InodeInformation.end())
            {
                std::string n = directoryTreeRec(num, 0);
                size_t pos = n.find_last_of("/");
                std::string dir = n.substr(0, pos);
                name += n;
                dir = "mkdir -p \"" + m_Helper.m_OutputFolder + dir + "\"";
                //std::cout << dir << " , " << name << std::endl;
                if (system(dir.c_str()));
            }
            else
            {
                ++notNamed;
                name += str + ".bin";
            }
        }
        int err = a_Reader.getInode(m_RegFiles.at(i));
        if (err != (int)m_Helper.m_ExtInodeSize)
        {
            std::cerr << "INODE_CARVER: Reader Error occurred." << std::endl;
            continue;
        }
        
        unsigned char inode[m_Helper.m_ExtInodeSize];
        memcpy(inode, a_Reader.m_InodeBuffer, m_Helper.m_ExtInodeSize);
        
        unsigned int flag = inode[32] | inode[33] << 8 | inode[34] << 16 | inode[35] << 24;
        unsigned int extent = flag & 0x80000;
        
        // If extents are used
        if (extent != 0)
        {
            // With using the 'y' option
            if (a_Simple == true)
            {
                //std::cout << "Relevant Inode found at address: " << m_RegFiles.at(i) << std::endl;
                m_Helper.setFileName(name);
                int retNum = m_Helper.extentRecursionRoot(a_Reader, m_RegFiles.at(i), true, m_InodeInformation, m_InodeTree, true);
                m_Helper.closeFile(name);
                if (retNum < 0)
                {
                    if (retNum == -6)
                        ++m_FoundDupliReg;
                    else
                        ++m_RegFailure;
                    remove(name.c_str());
                    //std::cerr << m_RegFiles.at(i) << " : Error occurred. 1" << std::endl;
                    continue;
                }
            }
            else                                            // Using the 'x' Option
            {
                //std::cout << "Relevant Inode found at address: " << m_RegFiles.at(i) << std::endl;
                m_Helper.setFileName(name);
                int retNum = m_Helper.extentRecursionRoot(a_Reader, m_RegFiles.at(i), true, m_InodeInformation, m_InodeTree, false);
                m_Helper.closeFile(name);
                if (retNum < 0)
                {
                    if (retNum == -6)
                        ++m_FoundDupliReg;
                    else
                        ++m_RegFailure;
                    remove(name.c_str());
                    //std::cerr << m_RegFiles.at(i) << " : Error occurred. 2" << std::endl;
                    continue;
                }
            }
        }
        else
        {
        // TODO No extents. But maybe something else
        // Other approach can be used here maybe...
        }
    }
    std::cout << "Not Named: " << notNamed << std::endl;
    
    // Creating empty directories and emtpy files
    for (auto it : m_InodeTree)
    {
        if (it.second.complete == false && m_Helper.m_FolderInodes.find(it.first) != m_Helper.m_FolderInodes.end())
        {
            std::string name = directoryTreeRec(it.first, 0);
            std::string dir = "mkdir -p \"" + m_Helper.m_OutputFolder + name + "\"";
            //std::cout << dir << " , " << name << std::endl;
            if (system(dir.c_str()));
        }
        else if (it.second.complete == false && m_Helper.m_FolderInodes.find(it.first) == m_Helper.m_FolderInodes.end())
        {
            // NOTE touch is only done to create empty files, so that the file names are at least shown
            std::string n = directoryTreeRec(it.first, 0);
            size_t pos = n.find_last_of("/");
            std::string dir = n.substr(0, pos);
            std::string name = n.substr(pos+1);
            pos = name.find_first_of("_");
            std::string num = name.substr(0, pos);
            name = name.substr(pos+1);
            std::string com = "touch \"" + m_Helper.m_OutputFolder + dir + "/" + num + "-nonRegular_" + name + "\"";
            //std::cout << dir << " , " << name << std::endl;
            if (system(com.c_str()));
        }
    }

}

/* Public function to gather information about inodes from the directory entries
 */
void InodeCarver::gatherInodeInformation(InodeFileReader &a_Reader)
{
    for (size_t i = 0; i < m_Directories.size(); ++i)
    {
        size_t num = m_Helper.determineInodeNumber(m_Directories.at(i));

        if (num == 0)
        {
            ++m_FoundDeletedDir;
            continue;
        }
        //std::cout << "Num: " << num << std::endl;
        int retNum = m_Helper.extentRecursionRoot(a_Reader, m_Directories.at(i), false, m_InodeInformation, m_InodeTree, false);
        if (retNum < 0)
        {
            if (retNum == -6)
                ++m_FoundDupliDir;
            else
                ++m_DirFailure;
            //std::cerr << m_Directories.at(i) << " : Error occurred. 3" << std::endl;
            return;
        }
    }
}

/* Private function to complete the directory tree in a recursive way
 */
std::string InodeCarver::directoryTreeRec(size_t a_InodeNum, size_t a_Depth)
{
    std::string ret = "";
    std::stringstream st;
    st << a_InodeNum;
    std::string t = st.str();
    
    // Loop detection
    if (a_Depth > m_InodeTree.size() || a_Depth > MAXTREEDEPTH)
    {
        std::cerr << "Found Loop." << std::endl;
        ret = "LoopDir_" + t;
        m_InodeTree[a_InodeNum].inodeParent = 0;
        m_InodeTree[a_InodeNum].complete = true;
        m_InodeTree[a_InodeNum].name = ret;
        return ret;
    }
    // Found a point in the tree, where the recursion could end
    if (m_InodeTree[a_InodeNum].complete == true)
    {
        //std::cout << "Reach end of Recursion." << std::endl;
        return m_InodeTree[a_InodeNum].name;
    }
    
    ret = directoryTreeRec(m_InodeTree[a_InodeNum].inodeParent, a_Depth+1);
    ret += "/" + t + "_" + m_InodeInformation[a_InodeNum];
    m_InodeTree[a_InodeNum].name = ret;
    m_InodeTree[a_InodeNum].complete = true;
    
    return ret;
}
