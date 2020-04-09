#include "include/InodeFileReader.h"

std::mutex HDD_Avail;

/* Constructor
 */
InodeFileReader::InodeFileReader()
    : m_Service(TskServices::Instance()),
    m_Image(m_Service.getImageFile()),
    m_CurrAddress(0)
{
    std::vector<std::string> test = m_Image.getFileNames();

    std::string out = m_Service.getSystemProperties().get(TskSystemProperties::PredefinedProperty::MODULE_OUT_DIR);

    out += "/InodeCarver/";
    
    m_OutPath = out;
    
    m_File.open(test.at(0).c_str(), std::ifstream::binary);
    m_CurrBuffer = new char[BLOCKSIZE];
    m_LoadBuffer = new char[BLOCKSIZE];

    m_Count = 0;
    m_Loaded = 0;
    m_ImgOffset = 0;
}

/* Destructor
 */
InodeFileReader::~InodeFileReader()
{
    m_File.close();
    if (m_CurrBuffer) delete [] m_CurrBuffer;
    if (m_LoadBuffer) delete [] m_LoadBuffer;
    if (m_InodeBuffer) delete [] m_InodeBuffer;
    if (m_DataBuffer) delete [] m_DataBuffer;
    if (m_DirectoryBuffer) delete [] m_DirectoryBuffer;
}

/* Public function to set the parameters and to initialize the buffer
 */
void InodeFileReader::initialize(size_t a_BlockSize, size_t a_InodeSize, size_t a_ImgOffset, size_t a_ImgSize)
{
    m_ExtBlockSize = a_BlockSize;
    m_ExtInodeSize = a_InodeSize;
    m_ImgOffset = a_ImgOffset;
    m_ImgSize = a_ImgSize;
    
    m_InodeBuffer = new char[m_ExtInodeSize];
    m_DirectoryBuffer = new char[m_ExtBlockSize];
    m_DataBuffer = new char[m_ExtBlockSize];
    
    m_File.seekg(m_CurrAddress + m_ImgOffset);
    m_File.read(m_LoadBuffer, BLOCKSIZE);
    m_Count = m_File.gcount();
    if (m_Count < BLOCKSIZE)
    {
        m_CurrAddress += m_Count;
    }
    else
    {
        m_CurrAddress += (BLOCKSIZE-m_ExtInodeSize);
    }
    m_Loaded = m_Count;
    if (m_CurrAddress > m_ImgSize)
    {
        std::cerr << "INODE_FILEREADER: Something wrong with initializing InodeFileReader. Maybe, Tool should be aborted." << std::endl;
    }
}

/* Public function to determine the size of the image
 */
size_t InodeFileReader::getRealImgSize()
{
    m_File.seekg (m_ImgOffset, m_File.end);
    size_t length = m_File.tellg();
    m_File.seekg (0, m_File.beg);
    
    return length;
}

/* Public function to return the path to the Sleuthkit-Output
 */
std::string InodeFileReader::getOutFolder()
{
    return m_OutPath;
}

/* Public function to get a new data block of the image
 */
int InodeFileReader::readNewBlock()
{
    //Lambda function
    auto lambda = [&] () -> int
    {
        HDD_Avail.lock();
        if (m_File.is_open())
        {
            m_File.seekg(m_CurrAddress + m_ImgOffset);
            m_File.read((char *)m_LoadBuffer, BLOCKSIZE);
            m_Count = m_File.gcount();
            if (m_Count < BLOCKSIZE)
            {
                //std::cout << "m_Count: " << m_Count << std::endl;
                m_CurrAddress += m_Count;
                //std::cout << "m_Curr new: " << m_CurrAddress << std::endl;
            }
            else
            {
                m_CurrAddress += (BLOCKSIZE-m_ExtInodeSize);
            }
        }
        HDD_Avail.unlock();
        m_localMtx.unlock();
        return m_Count;
    };

    //Swap buffers
    m_localMtx.lock();
    char *tmp = m_LoadBuffer;
    m_LoadBuffer = m_CurrBuffer;
    m_CurrBuffer = tmp;
    m_Loaded = m_Count;
    
    //Reload buffer async
    if (m_CurrAddress < (m_ImgSize+m_ImgOffset))
    {
        m_threadSync = std::async(std::launch::async, lambda);
    }
    else
    {
        //std::cout << "Adress: " << m_CurrAddress << std::endl;
        //return -1;
    }
    return m_Loaded;
}

/* Public function to get the data of an inode
 */
int InodeFileReader::getInode(size_t a_Address)
{
    m_File.clear();
    if (a_Address >= m_ImgSize)
    {
        std::cerr << "INODE_FILEREADER: Reached end of Image." << std::endl;
        return -1;
    }
    m_File.seekg(a_Address + m_ImgOffset);
    m_File.read(m_InodeBuffer, m_ExtInodeSize);
    if (m_File.gcount() < (int)m_ExtInodeSize)
    {
        std::cerr << "INODE_FILEREADER " << a_Address << " : Something went wrong with getting inode data: " << m_File.gcount() << std::endl;
    }
    return m_File.gcount();
}

/* Public function to get the data of an inode
 */
int InodeFileReader::getDirectory(size_t a_Address)
{
    m_File.clear();
    if (a_Address >= m_ImgSize)
    {
        std::cerr << "INODE_FILEREADER: Reached end of Image." << std::endl;
        return -1;
    }
    m_File.seekg(a_Address + m_ImgOffset);
    m_File.read(m_DirectoryBuffer, m_ExtBlockSize);
    if (m_File.gcount() < (int)m_ExtBlockSize)
    {
        std::cerr << "INODE_FILEREADER " << a_Address << " : Something went wrong with getting directory data: " << m_File.gcount() << std::endl;
    }
    return m_File.gcount();
}

/* Public function to get a data block of the image
 */
int InodeFileReader::getData(size_t a_Address)
{
    m_File.clear();
    if (a_Address >= m_ImgSize)
    {
        std::cerr << "INODE_FILEREADER: Reached end of Image." << std::endl;
        return -1;
    }
    m_File.seekg(a_Address + m_ImgOffset);
    m_File.read(m_DataBuffer, m_ExtBlockSize);
    if (m_File.gcount() < (int)m_ExtBlockSize)
    {
        std::cerr << "INODE_FILEREADER: Something went wrong with getting data: " << m_File.gcount() << std::endl;
    }
    return m_File.gcount();
}

/* Public function to reset the state of the file reader
 */
void InodeFileReader::reset()
{
    m_File.clear();
    m_File.seekg(m_ImgOffset, m_File.beg);
    m_CurrAddress = 0;
    m_File.seekg(m_CurrAddress + m_ImgOffset);
    m_File.read(m_LoadBuffer, BLOCKSIZE);
    m_Count = m_File.gcount();
    if (m_Count < BLOCKSIZE)
    {
        m_CurrAddress += m_Count;
    }
    else
    {
        m_CurrAddress += (BLOCKSIZE-m_ExtInodeSize);
    }
    m_Loaded = m_Count;
}

/* Public getter of block size
 */
size_t InodeFileReader::getBlockSize()
{
    return BLOCKSIZE;
}
