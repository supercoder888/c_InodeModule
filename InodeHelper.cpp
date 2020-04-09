#include "include/InodeHelper.h"

/* Constructor
 */
InodeHelper::InodeHelper()
{
    // Set default values
    m_OutputFolder = ".";
    m_ExtBlockSize = 0;
    m_ExtBlockGroupSize = 0;
    m_ExtInodeSize = 0;
    m_ExtBlockCount = 0;
    m_ImgOffset = 0;
    m_FlexGroupSize = 16;
    m_64Bit = 32;
    m_Sparse = true;
    m_InodeRatio = 0;
    m_TimeStampMin = 0;
    m_TimeStampMax = 4294967295;
    m_TimeStampSet = false;
    
    m_DataBuffer = 0;
    m_IsRoot = false;
    m_Parent = 0;
    m_TempNum = 0;
    
    m_Rest = 0;
}

/* Destructor
 */
InodeHelper::~InodeHelper()
{
    if (m_DataBuffer != 0)
        delete [] m_DataBuffer;
}

/* Public function to set the argument with the path to the config file.
 */
bool InodeHelper::setArgument(std::string a_Path)
{
    std::ifstream file(a_Path.c_str());
    std::string temp;
    size_t imgSize = 0;

    // Interpreting arguments
    while (file.good())
    {
        file >> temp;
        if (temp == "$OutputFolder")
        {
            file >> temp;
            m_OutputFolder = temp;
        }
        else if (temp == "$ImageSize")
        {
            file >> temp;
            imgSize = strtoull(temp.c_str(), NULL, 0);
            if (m_ImgSize != imgSize)
            {
                std::cout << "INODE_HELPER: Warning: File has different size than expected. Maybe an error? Size: " << m_ImgSize << std::endl;
            }
            m_ImgSize = imgSize;
        }
        else if (temp == "$BlockSize")
        {
            file >> temp;
            m_ExtBlockSize = atoi(temp.c_str());
            m_ExtBlockGroupSize = m_ExtBlockSize * 8;
        }
        else if (temp == "$InodeSize")
        {
            file >> temp;
            m_ExtInodeSize = atoi(temp.c_str());
        }
        else if (temp == "$ImgOffset")
        {
            file >> temp;
            m_ImgOffset = atoi(temp.c_str());
        }
        else if (temp == "$FlexGroupSize")
        {
            file >> temp;
            m_FlexGroupSize = atoi(temp.c_str());
            if (m_FlexGroupSize == 0)
            {
                m_FlexGroupSize = 1;
            }
        }
        else if (temp == "$64BitEnable")
        {
            file >> temp;
            if (temp == "true")
            {
                m_64Bit = 64;
            }
            else
            {
                m_64Bit = 32;
            }
        }
        else if (temp == "$SparseSuperblock")
        {
            file >> temp;
            if (temp == "true")
            {
                m_Sparse = true;
            }
            else
            {
                m_Sparse = false;
            }
        }
        else if (temp == "$InodeRatio")
        {
            file >> temp;
            m_InodeRatio = atoi(temp.c_str());
        }
        else if (temp == "$TimeStampMin")
        {
            file >> temp;
            m_TimeStampMin = strtoull(temp.c_str(), NULL, 0);
            m_TimeStampSet = true;
        }
        else if (temp == "$TimeStampMax")
        {
            file >> temp;
            m_TimeStampMax = strtoull(temp.c_str(), NULL, 0);
            m_TimeStampSet = true;
        }
        else if (temp == "$Permission")
        {
            file >> temp;
            unsigned short s = strtoul(temp.c_str(), NULL, 0);
            if (s > 0x1ff)
            {
                std::cerr << "INODE_HELPER: This permission " << s << " was out of bound." << std::endl;
                continue;
            }
            m_Permission.push_back(s);
        }
        else if (temp.front() == '-')
        {
            std::cout << "INODE_HELPER: Ignored: " << temp << std::endl;
            file >> temp;
            continue;
        }
        else
        {
            break;
        }
    }
    
    std::cout << m_OutputFolder << std::endl;
    std::string a = "mkdir -p \"" + m_OutputFolder + "\"";
    if (system(a.c_str()) !=  0)
    {
        std::cerr << "INODE_HELPER: Output folder could not be created. Error!" << std::endl;
        return false;
    }
    
    if (imgSize == 0)
    {
        std::cout << "Self determined file size is taken: " << m_ImgSize << std::endl;
        m_ImgSize -= m_ImgOffset;
    }
    
    setDefault();
    
    m_ExtBlockCount = m_ImgSize / m_ExtBlockSize;
    if (m_ImgSize % m_ExtBlockSize != 0)
    {
        ++m_ExtBlockCount;
    }
    //std::cout << m_ImgSize << " " << m_ExtBlockCount << std::endl;
    
    calculateImageParameters();
    return true;
}

/* Public function to open file
 */
void InodeHelper::setFileName(std::string a_Name)
{
    m_File.open(a_Name);
}

/* Public function to close the open file
 */
void InodeHelper::closeFile(std::string a_Name)
{
    m_File.seekp (0, m_File.end);
    size_t length = m_File.tellp();
    //std::cout << "Length: " << length << " , Rest: " << m_Rest << std::endl;
    m_File.seekp (0, m_File.beg);
    
    m_File.close();
    
    // File to big, has to be resized
    long long diff = (long long)length - (long long)m_Rest;
    //std::cout << "Diff: " << diff << std::endl;
    if (m_Rest != 0 && diff > 0)
    {
        if (truncate(a_Name.c_str(), length - m_Rest) < 0)
        {
            std::cerr << "INODE_HELPER: Length: " << length << " , Rest: " << m_Rest << std::endl;
            std::cerr << "INODE_HELPER: Failure truncating file happened." <<  std::endl;
        }
    }
}

/* Public function to map physical addresses to inode numbers
 */
size_t InodeHelper::determineInodeNumber(size_t a_Address)
{
    // Values for this physical address
    size_t blockAd = a_Address / m_ExtBlockSize;
    size_t blockGroup = blockAd / m_ExtBlockGroupSize;
    
    size_t flex = blockGroup / m_FlexGroupSize;
    size_t offset = flex * m_FlexGroupSize;

    if (blockGroup != offset)
    {
        return 0;
    }

    // Only case when sparse mode is relevant
    size_t result = 0;
    if (m_Sparse == true)
    {
        if (m_FlexGroupSize == 1)
        {
            // 0 - 1 - 3 - 5 - 7
            if (blockGroup == 0 || blockGroup == 1)
            {
                result += m_SuperGrowth;
            }
            else
            {
                float loggy = std::log(blockGroup);
                size_t l = loggy / std::log(3.0f);
                size_t check = std::pow(3.0f, l);
                if (check == blockGroup)
                {
                    result += m_SuperGrowth;
                }
                l = loggy / std::log(5.0f);
                check = std::pow(5.0f, l);
                if (check == blockGroup)
                {
                    result += m_SuperGrowth;
                }
                l = loggy / std::log(7.0f);
                check = std::pow(7.0f, l);
                if (check == blockGroup)
                {
                    result += m_SuperGrowth;
                }
            }
        }
        else
        {
            // case block 0
            if (blockGroup == 0)
                result += m_SuperGrowth;
        }
    }
    else
    {
        result += m_SuperGrowth;
    }
    result += (offset * m_ExtBlockGroupSize);
    result += m_InodeTableOffset;
    result *= m_ExtBlockSize;
    size_t diff = a_Address - result;
    diff /= m_ExtInodeSize;
    
    if (diff >= (m_NumOfInodesPerGroup*m_FlexGroupSize))
    {
        return 0;
    }
    result = diff+1 + (m_NumOfInodesPerGroup * blockGroup);
    
    if (result >= (m_NumOfInodesPerGroup * m_NumberOfBlockGroups))
    {
        return 0;
    }

    //std::cout << "InodeTableOff: " << m_InodeTableOffset << " Num of Inode: " << m_NumOfInodesPerGroup << std::endl;
    
    return result;
}

/* Public function to handle extent depth > 0. This is called from root.
 */
int InodeHelper::extentRecursionRoot(InodeFileReader &a_Reader, size_t a_Address, bool a_IsFile, std::map<size_t, std::string> &a_InodeInformation, std::map<size_t, directoryTree> &a_InodeTree, bool a_Simple)
{
    // Set initial values
    if (m_DataBuffer == 0)
        m_DataBuffer = new unsigned char[m_ExtBlockSize];
    
    m_TempTree.clear();
    m_TempInodeInformation.clear();
    m_TempNum = 0;
    m_Parent = 0;
    m_IsRoot = false;
    size_t fSize = 0;
    
    // Get the inode-data
    int err = a_Reader.getInode((a_Address));
    if (err != (int)m_ExtInodeSize)
    {
        std::cerr << "INODE_HELPER " << a_Address << " : Reading Data Block went wrong." << std::endl;
        return -1;
    }
    
    // Determine filesize and get other flags
    if (a_IsFile == true)
    {
        fSize = (unsigned char)a_Reader.m_InodeBuffer[4] | (unsigned char)a_Reader.m_InodeBuffer[5] << 8 | (unsigned char)a_Reader.m_InodeBuffer[6] << 16 | (unsigned char)a_Reader.m_InodeBuffer[7] << 24;
        size_t rest = fSize % m_ExtBlockSize;
        if (rest != 0)
        {
            m_Rest = m_ExtBlockSize-rest;
        }
        else
        {
            m_Rest = 0;
        }
    }
    
    unsigned int flag = (unsigned char)a_Reader.m_InodeBuffer[32] | (unsigned char)a_Reader.m_InodeBuffer[33] << 8 | (unsigned char)a_Reader.m_InodeBuffer[34] << 16 | (unsigned char)a_Reader.m_InodeBuffer[35] << 24;
    unsigned int dirHash = flag & 0x1000;
    
    bool isDirHash = false;
    if (dirHash != 0)
        isDirHash = true;

    memcpy(m_DataBuffer, a_Reader.m_InodeBuffer+40, 60);
    unsigned short magic = m_DataBuffer[0] | m_DataBuffer[1] << 8;
    
    // Remove duplicate inodes
    if (a_Simple == true)
    {
        size_t up = (unsigned char)a_Reader.m_InodeBuffer[108] | (unsigned char)a_Reader.m_InodeBuffer[109] << 8 | (unsigned char)a_Reader.m_InodeBuffer[110] << 16 | (unsigned char)a_Reader.m_InodeBuffer[111] << 24;
        size_t size = (size_t)fSize | (size_t)up << 32;
        
        ext4Bytes tmp;
        memcpy(tmp.a, a_Reader.m_InodeBuffer+40, 60);

        for (size_t run = 0; run < m_RecoveredInodes.size(); ++run)
        {
            if ((std::equal(tmp.a, tmp.a+60, m_RecoveredInodes[run].a) == true) && (size == m_RecoveredInodes[run].size))
            {
                //std::cout << "Dupli Inode! " << size << std::endl;
                return -6;
            }
        }
        tmp.size = size;
        m_RecoveredInodes.push_back(tmp);
        
    }
   // Error cases
    if (magic != 0xF30A)
    {
        // ERROR!
        //std::cerr << "INODE_HELPER " << a_Address << " : Extent lead to wrong block address. (No magic number, root)" << std::endl;
        return -2;
    }
    unsigned short entries = m_DataBuffer[2] | m_DataBuffer[3] << 8;
    unsigned short depth = m_DataBuffer[6] | m_DataBuffer[7] << 8;
    if (depth > 5)
    {
        //std::cerr << "INODE_HELPER " << a_Address << " : Extent lead to wrong block address. (Unexpected depth, root)" << std::endl;
        return -3;
    }
    if (entries > 4)                                        // || entries == 0)
    {
        //std::cerr << "INODE_HELPER " << a_Address << " : Extent lead to wrong block address. (Invalid entry count, root) Maybe deleted inode." << std::endl;
        return -4;
    }
    
    //std::cout << "\tEntries: " << entries << " , Depth: " << depth << std::endl;
    
    // Go into recursion
    int retNum = extentRecursion(a_Reader, a_Address, depth, depth, a_IsFile, isDirHash);
    if (retNum == 0)
    {
        size_t up = (unsigned char)a_Reader.m_InodeBuffer[108] | (unsigned char)a_Reader.m_InodeBuffer[109] << 8 | (unsigned char)a_Reader.m_InodeBuffer[110] << 16 | (unsigned char)a_Reader.m_InodeBuffer[111] << 24;
        if (up == 0 && fSize == 0)
        {}
        else
        {
            size_t size = (size_t)fSize | (size_t)up << 32;
            std::cerr << size << " " << fSize << " " << up << std::endl;
            std::cerr << "INODE_HELPER: " << a_Address << " : Inode found at this address with no entries in the extent structure, but with a file size > 0. The size was: " << size << std::endl;
        }
    }
    
    // Check if everything  went fine
    if (retNum >= 0 && a_IsFile == false)
    {        
        if (m_IsRoot == true && m_TempNum == 0)
        {
            // ERROR
            //std::cerr << "INODE_HELPER " << a_Address << " : m_TempNum wrong? " << m_TempNum << std::endl; 
            return -7;
        }
        else if (m_IsRoot == false && (m_Parent == 0 || m_TempNum == 0))
        {
            //std::cerr << "INODE_HELPER " << a_Address << " : m_TempNum wrong? " << m_TempNum << " , Parent: " << m_Parent << std::endl; 
            return -7;
            // ERROR
        }
        
        for (auto &it : m_TempTree)
        {
            if (m_IsRoot == true && it.second.complete == true)
            {}
            else
                it.second.inodeParent = m_TempNum;
        }

        a_InodeTree.insert(m_TempTree.begin(), m_TempTree.end());
        a_InodeInformation.insert(m_TempInodeInformation.begin(), m_TempInodeInformation.end());

        m_FolderInodes.insert(m_TempNum);
    }
    
    return retNum;
}

/* Private function to handle extent depth > 0
 */
int InodeHelper::extentRecursion(InodeFileReader &a_Reader, size_t a_Address, size_t a_ExpectedDepth, size_t a_MaxDepth, bool a_IsFile, bool a_IsDirHash)
{
    unsigned short entries = 0;
    unsigned short depth = 0;
    
    // If not the init case,  do the checks
    if (a_ExpectedDepth != a_MaxDepth)
    {
        int err = a_Reader.getData((a_Address*m_ExtBlockSize));
        if (err != (int)m_ExtBlockSize)
        {
            std::cerr << "INODE_HELPER " << a_Address << " : Reading Data Block went wrong." << std::endl;
            return -1;
        }
        memcpy(m_DataBuffer, a_Reader.m_DataBuffer, m_ExtBlockSize);
        unsigned short magic = m_DataBuffer[0] | m_DataBuffer[1] << 8;
        if (magic != 0xF30A)
        {
            // ERROR!
            //std::cerr << "INODE_HELPER " << a_Address << " : Extent lead to wrong block address. (No magic number)" << std::endl;
            return -2;
        }
        entries = m_DataBuffer[2] | m_DataBuffer[3] << 8;
        depth = m_DataBuffer[6] | m_DataBuffer[7] << 8;
        //std::cout << "\tEntries: " << entries << " , Depth: " << depth << std::endl;
        
        if (depth != a_ExpectedDepth)
        {
            //std::cerr << "INODE_HELPER " << a_Address << " : Extent lead to wrong block address. (Unexpected depth)" << std::endl;
            return -3;
        }
        
        if (entries > (m_ExtBlockSize/12) || entries == 0)
        {
            //std::cerr << "INODE_HELPER " << a_Address << " : Extent lead to wrong block address. (Invalid entry count)" << std::endl;
            return -4;
        }
    }
    else
    {
        depth = a_ExpectedDepth;
        entries = m_DataBuffer[2] | m_DataBuffer[3] << 8;
        
        if (entries == 0)
        {
            return 0;
        }
        
    }
    
    size_t currentBlockNumber = 0;
    size_t count = 0, count2 = 0;
    size_t curMin = 32768*700;
    bool curMinSet = false;
    
    while (1)
    {
        // Search for correct entry
        for (size_t ent = 0; ent < entries; ++ent)
        {
            unsigned int block = m_DataBuffer[12+(ent*12)] | m_DataBuffer[13+(ent*12)] << 8 |
            m_DataBuffer[14+(ent*12)] << 16 | m_DataBuffer[15+(ent*12)] << 24;
            
            if (block > currentBlockNumber && block <= curMin)
            {
                curMin = block;
                curMinSet = false;
            }
            
            if (block != currentBlockNumber)
            {
                continue;
            }
            
            // Reached the leaves
            if (depth == 0)
            {
                unsigned short length = m_DataBuffer[16+(ent*12)] | m_DataBuffer[17+(ent*12)] << 8;
                unsigned long long ad = (unsigned long long)m_DataBuffer[20+(ent*12)] | (unsigned long long)m_DataBuffer[21+(ent*12)] << 8 |
                (unsigned long long)m_DataBuffer[22+(ent*12)] << 16 | (unsigned long long)m_DataBuffer[23+(ent*12)] << 24 |
                (unsigned long long)m_DataBuffer[18+(ent*12)] << 32 | (unsigned long long)m_DataBuffer[19+(ent*12)] << 40;
                if (length > 32768)
                {
                    //std::cerr << "INODE_HELPER " << a_Address << " : Uninitialized Extent." << std::endl;
                    currentBlockNumber += (length - 32768);
                    break;
                }
                // length of extent entry
                for (size_t b = 0; b < length; ++b)
                {
                    if (a_IsFile == true)
                    {
                        int err = a_Reader.getData(((ad+b)*m_ExtBlockSize));
                        if (err != (int)m_ExtBlockSize)
                        {
                            std::cerr << "INODE_HELPER " << a_Address << " : Reading Data Block went wrong." << std::endl;
                            return -5;
                        }
                        m_File.write(a_Reader.m_DataBuffer, m_ExtBlockSize);
                    }
                    else
                    {
                        a_Reader.getDirectory(((ad+b)*m_ExtBlockSize));
                        int retNum = readDirectoryEntry(a_Reader, a_IsDirHash);
                        if (retNum < 0)
                        {
                            return -8;
                        }
                    }
                }
                
                currentBlockNumber += length;
                curMin = 32768*700;
                curMinSet = true;
                ++count2;
            }
            else                                            // Go on with recursion
            {
                unsigned long long ad = (unsigned long long)m_DataBuffer[16+(ent*12)] | (unsigned long long)m_DataBuffer[17+(ent*12)] << 8 |
                (unsigned long long)m_DataBuffer[18+(ent*12)] << 16 | (unsigned long long)m_DataBuffer[19+(ent*12)] << 24 |
                (unsigned long long)m_DataBuffer[20+(ent*12)] << 32 | (unsigned long long)m_DataBuffer[21+(ent*12)] << 40;

                size_t retNum = extentRecursion(a_Reader, ad, a_ExpectedDepth-1, a_MaxDepth, a_IsFile, a_IsDirHash);
                if (retNum < 0)
                {
                    return retNum;
                }
                currentBlockNumber += retNum;
                curMin = 32768*700;
                curMinSet = true;
                ++count2;
            }
        }
        
        // Break-cases
        if (count2 == entries)
        {
            break;
        }
        if (curMinSet == true)
        {
            curMinSet = false;
            continue;
        }
        if (curMin > currentBlockNumber)
        {
            
            if (curMin == 32768*700)
                break;
            
            // Fill file with empty blocks
            if (a_IsFile == true)
            {
                unsigned char *arr = new unsigned char[m_ExtBlockSize];
                memset(arr, 0, m_ExtBlockSize);
                for (size_t i = currentBlockNumber; i < curMin; ++i)
                {
                    m_File.write((char *)arr, m_ExtBlockSize);
                }
            }
            currentBlockNumber = curMin;
            curMin = 32768*700;
        }
        ++count;
        if (count >= 2*entries)
        {
            break;
        }
    }

    return currentBlockNumber;
}

/* Public function to read the directory entries
 */
int InodeHelper::readDirectoryEntry(InodeFileReader &a_Reader, bool a_IsDirHash)
{
    size_t off = 0;
    size_t type = 0;
    size_t nameLen1 = 0;
    size_t nameLen = 0;
    size_t inodeNum = 0;

    inodeNum = (unsigned char)a_Reader.m_DirectoryBuffer[off+0] | (unsigned char)a_Reader.m_DirectoryBuffer[off+1] << 8 | (unsigned char)a_Reader.m_DirectoryBuffer[off+2] << 16 | (unsigned char)a_Reader.m_DirectoryBuffer[off+3] << 24;
    
    // Go over all directory entries
    while (1)
    {
        size_t recLen = (unsigned char)a_Reader.m_DirectoryBuffer[off+4] | (unsigned char)a_Reader.m_DirectoryBuffer[off+5] << 8;
        type = (unsigned char)a_Reader.m_DirectoryBuffer[off+7];
        // interpret directory entries different
        if (type != 0)
        {
            nameLen = (unsigned char)a_Reader.m_DirectoryBuffer[off+6];
        }
        else
        {
            nameLen = (unsigned char)a_Reader.m_DirectoryBuffer[off+6] | (unsigned char)a_Reader.m_DirectoryBuffer[off+7] << 8;
        }

        std::string n(a_Reader.m_DirectoryBuffer+off+8, nameLen);
        size_t pos = n.find_first_of('\0');
        std::string name = n.substr(0, pos);

        nameLen1 = recLen - 8;
        
        // Found valid inode
        if (inodeNum != 0)
        {
            // Normal case, if not root directory
            if (m_TempInodeInformation.find(inodeNum) == m_TempInodeInformation.end() && m_TempNum != inodeNum)
            {
                if (name == ".")
                {
                    m_TempNum = inodeNum;
                }
                else if (name == "..")
                {
                    m_Parent = inodeNum;
                }
                else
                {
                    m_TempInodeInformation[inodeNum] = name;
                    //std::cout << "Num: " << inodeNum << " , Name: " << name << std::endl;
                    m_TempTree[inodeNum].complete = false;
                }
            }
            else
            {
                // Found root directory
                if (m_TempNum == inodeNum && name == "..")
                {
                    //std::cout << "INODE_HELPER: Root found: " << inodeNum << std::endl;
                    std::stringstream st;
                    st << inodeNum;
                    std::string t = st.str();
                    m_TempInodeInformation[inodeNum] = "rootDir_" + t;
                    m_TempTree[inodeNum].complete = true;
                    m_TempTree[inodeNum].name = "rootDir_" + t;
                    m_TempTree[inodeNum].inodeParent = 0;
                    m_Parent = 0;
                    m_TempNum = inodeNum;
                    m_IsRoot = true;
                }
                else
                {
                    std::cerr << "INODE_HELPER: Input-Error: " << inodeNum << " , " << name << std::endl;
                    return -1;
                }
            }
        }
        else
        {
            //std::cout << "Deleted file" << std::endl;
        }
        if (nameLen1 == (m_ExtBlockSize-off-8))
        {
            break;
        }
        else if (recLen > 263)
        {
            //std::cout << "Skipped gap!" << std::endl;
        }

        off += recLen;
        inodeNum = (unsigned char)a_Reader.m_DirectoryBuffer[off+0] | (unsigned char)a_Reader.m_DirectoryBuffer[off+1] << 8 | (unsigned char)a_Reader.m_DirectoryBuffer[off+2] << 16 | (unsigned char)a_Reader.m_DirectoryBuffer[off+3] << 24;

        if (inodeNum == 0)
        {
            //std::cout << "Yey, delete!" << std::endl;
        }

        if ((nameLen > 255) || (off > m_ExtBlockSize-12) || (recLen == 0))
            break;
    }

    return 0;
}

/* Private function to calculate ext4 image parameters only once
 */
void InodeHelper::calculateImageParameters()
{
    m_NumberOfBlockGroups = m_ExtBlockCount / m_ExtBlockGroupSize;
    if (m_ExtBlockCount % m_ExtBlockGroupSize != 0)
    {
        ++m_NumberOfBlockGroups;
    }
    m_SuperGrowth = 1;
    size_t temp =(m_NumberOfBlockGroups*m_64Bit) / m_ExtBlockSize;
    if (((m_NumberOfBlockGroups*m_64Bit) % m_ExtBlockSize) != 0)
    {
        ++temp;
    }

    size_t maxBlocks = std::pow(2, 32);
    if (m_ExtBlockCount < std::pow(2, 22))
    {
        maxBlocks = m_ExtBlockCount * 1024;
    }
    size_t dpb = m_ExtBlockSize / m_64Bit;
    size_t value = std::ceil((float)maxBlocks / (float)m_ExtBlockGroupSize);
    size_t t = std::ceil((float)value / (float)dpb) - temp;
    if (t > (m_ExtBlockSize/(m_64Bit/8)))
    {
        t = (m_ExtBlockSize/(m_64Bit/8));
    }
    m_SuperGrowth += t;
    m_SuperGrowth += temp;

    m_InodeTableOffset = (2*m_FlexGroupSize);
    
    if (m_ExtBlockSize <= 1024)
        m_InodeTableOffset += 1;

    m_NumOfInodesPerGroup = m_ExtBlockCount * m_ExtBlockSize;
    m_NumOfInodesPerGroup /= m_InodeRatio;
    //Number of Inodes 
    m_NumOfInodesPerGroup /= m_NumberOfBlockGroups;
    //Number of Inodes per BG 
    size_t inodesPerBlock = m_ExtBlockSize / m_ExtInodeSize;
    //std::cout << "Inodes per Group: " << m_NumOfInodesPerGroup << std::endl;
    m_NumOfInodesPerGroup = roundToNext(m_NumOfInodesPerGroup, inodesPerBlock);

}

/* Private function to set default values
 */
void InodeHelper::setDefault()
{
    std::cout << "Type: ";
    if (m_ImgSize < 3145728)
    {
        // floppy
        if (m_ExtBlockSize == 0)
        {
            m_ExtBlockSize = 1024;
            m_ExtBlockGroupSize = m_ExtBlockSize * 8;
        }
        
        if (m_ExtInodeSize == 0)
        {
            m_ExtInodeSize = 128;
        }
        
        if (m_InodeRatio == 0)
        {
            m_InodeRatio = 8192;
        }
        
        std::cout << "Floppy" << std::endl;
    }
    else if (m_ImgSize < 536870912)
    {
        // small
        if (m_ExtBlockSize == 0)
        {
            m_ExtBlockSize = 1024;
            m_ExtBlockGroupSize = m_ExtBlockSize * 8;
        }

        if (m_ExtInodeSize == 0)
        {
            m_ExtInodeSize = 128;
        }

        if (m_InodeRatio == 0)
        {
            m_InodeRatio = 4096;
        }
        
        std::cout << "Small" << std::endl;
    }
    else if ((m_ImgSize/(1024*1024) < 4194304))
    {
        // default
        if (m_ExtBlockSize == 0)
        {
            m_ExtBlockSize = 4096;
            m_ExtBlockGroupSize = m_ExtBlockSize * 8;
        }

        if (m_ExtInodeSize == 0)
        {
            m_ExtInodeSize = 256;
        }

        if (m_InodeRatio == 0)
        {
            m_InodeRatio = 16384;
        }
        
        std::cout << "Default" << std::endl;
    }
    else if ((m_ImgSize/(1024*1024) < 16777216))
    {
        // big
        if (m_ExtBlockSize == 0)
        {
            m_ExtBlockSize = 4096;
            m_ExtBlockGroupSize = m_ExtBlockSize * 8;
        }

        if (m_ExtInodeSize == 0)
        {
            m_ExtInodeSize = 256;
        }

        if (m_InodeRatio == 0)
        {
            m_InodeRatio = 32768;
        }
        std::cout << "Big" << std::endl;
    }
    else if ((m_ImgSize/(1024*1024) >= 16777216))
    {
        // huge
        if (m_ExtBlockSize == 0)
        {
            m_ExtBlockSize = 4096;
            m_ExtBlockGroupSize = m_ExtBlockSize * 8;
        }

        if (m_ExtInodeSize == 0)
        {
            m_ExtInodeSize = 256;
        }

        if (m_InodeRatio == 0)
        {
            m_InodeRatio = 65536;
        }
        
        std::cout << "Huge" << std::endl;
    }
    else
    {
        std::cerr << "INODE_HELPER: Type could not be determined. Default values taken" << std::cout;
        if (m_ExtBlockSize == 0)
        {
            m_ExtBlockSize = 4096;
            m_ExtBlockGroupSize = m_ExtBlockSize * 8;
        }

        if (m_ExtInodeSize == 0)
        {
            m_ExtInodeSize = 256;
        }

        if (m_InodeRatio == 0)
        {
            m_InodeRatio = 16384;
        }
    }
}

/* Private function for round operations
 */
size_t InodeHelper::roundToFour(size_t a_Value)
{
    size_t val = a_Value;
    size_t mod = a_Value % 4;
    if (mod != 0)
    {
        val += 4;
        val -= mod;
    }
    
    return val;
}

/* Private function for round operations
 */
size_t InodeHelper::roundToNext(size_t a_Value, size_t a_Round)
{
    size_t val = a_Value;
    size_t mod = a_Value % a_Round;
    if (mod != 0)
    {
        val += a_Round;
        val -= mod;
    }

    return val;
}
