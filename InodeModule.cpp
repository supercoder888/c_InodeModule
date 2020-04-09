/*
*
*  The Sleuth Kit
*
*  Contact: Brian Carrier [carrier <at> sleuthkit [dot] org]
*
*  This is free and unencumbered software released into the public domain.
*
*  Anyone is free to copy, modify, publish, use, compile, sell, or
*  distribute this software, either in source code form or as a compiled
*  binary, for any purpose, commercial or non-commercial, and by any
*  means.
*
*  In jurisdictions that recognize copyright laws, the author or authors
*  of this software dedicate any and all copyright interest in the
*  software to the public domain. We make this dedication for the benefit
*  of the public at large and to the detriment of our heirs and
*  successors. We intend this dedication to be an overt act of
*  relinquishment in perpetuity of all present and future rights to this
*  software under copyright law.
*
*  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
*  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
*  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
*  IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
*  OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
*  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
*  OTHER DEALINGS IN THE SOFTWARE.
*/

/**
* \file InodeModule.cpp
* Contains the implementation of a file analysis module that tries to
* reconstruct files with only using Inodes.
*/

// TSK Framework includes
#include "tsk/framework/utilities/TskModuleDev.h"
#include "tsk/framework/services/TskServices.h"
#include "include/InodeFileReader.h"
#include "include/InodeCarver.h"

// Poco includes
// Uncomment this include if using the Poco catch blocks.
//#include "Poco/Exception.h"

// C/C++ library includes
#include <string>
#include <cstring>
#include <sstream>
#include <iostream>
#include <math.h>
#include <assert.h>

// More complex modules will likely put functions and variables other than
// the module API functions in separate source files and/or may define various
// C++ classes to perform the work of the module. However, it is possible to simply
// enclose such functions and variables in an anonymous namespace to give them file scope
// instead of global scope, as is done in this module. This replaces the older practice
// of declaring file scope functions and variables using the "static" keyword. An
// anonymous namespace is a more flexible construct, since it is possible to define
// types within it.
//
// NOTE: Linux/OS-X module developers should make sure module functions
// other than the module API functions are either uniquely named or bound at module link time.
// Placing these functions in an anonymous namespace to give them static-linkage is one way to
// accomplish this.
//
// CAVEAT: Static data can be incompatible with multithreading, since each
// thread will get its own copy of the data.
namespace
{
    const char *MODULE_NAME = "tskInodeModule";
    const char *MODULE_DESCRIPTION = "Performs an inode calculation for the contents of a given file";
    const char *MODULE_VERSION = "1.0.0";
    
    std::string m_Argument;
}

extern "C"
{
    /**
    * Module identification function.
    *
    * CAVEAT: This function is intended to be called by TSK Framework only.
    * Linux/OS-X modules should *not* call this function within the module
    * unless appropriate compiler/linker options are used to bind all
    * library-internal symbols at link time.
    *
    * @return The name of the module.
    */
    TSK_MODULE_EXPORT const char *name()
    {
        return MODULE_NAME;
    }

    /**
    * Module identification function.
    *
    * CAVEAT: This function is intended to be called by TSK Framework only.
    * Linux/OS-X modules should *not* call this function within the module
    * unless appropriate compiler/linker options are used to bind all
    * library-internal symbols at link time.
    *
    * @return A description of the module.
    */
    TSK_MODULE_EXPORT const char *description()
    {
        return MODULE_DESCRIPTION;
    }

    /**
    * Module identification function.
    *
    * CAVEAT: This function is intended to be called by TSK Framework only.
    * Linux/OS-X modules should *not* call this function within the module
    * unless appropriate compiler/linker options are used to bind all
    * library-internal symbols at link time.
    *
    * @return The version of the module.
    */
    TSK_MODULE_EXPORT const char *version()
    {
        return MODULE_VERSION;
    }

    /**
    * Module initialization function. Receives a string of initialization arguments,
    * typically read by the caller from a pipeline configuration file.
    * Returns TskModule::OK or TskModule::FAIL. Returning TskModule::FAIL indicates
    * the module is not in an operational state.
    *
    * CAVEAT: This function is intended to be called by TSK Framework only.
    * Linux/OS-X modules should *not* call this function within the module
    * unless appropriate compiler/linker options are used to bind all
    * library-internal symbols at link time.
    *
    * @param args a string of initialization arguments.
    * @return TskModule::OK if initialization succeeded, otherwise TskModule::FAIL.
    */
    TskModule::Status TSK_MODULE_EXPORT initialize(const char* arguments)
    {
        // The TSK Framework convention is to prefix error messages with the
        // name of the module/class and the function that emitted the message.
        std::ostringstream msgPrefix;
        msgPrefix << MODULE_NAME << "::initialize : ";

        // Well-behaved modules should catch and log all possible exceptions
        // and return an appropriate TskModule::Status to the TSK Framework.
        try
        {
            // If this module required initialization, the initialization code would
            // go here.
            m_Argument = arguments;
            

            return TskModule::OK;
        }
        catch (TskException &ex)
        {
            std::ostringstream msg;
            msg << msgPrefix.str() << "TskException: " << ex.message();
            LOGERROR(msg.str());
            return TskModule::FAIL;
        }
        // Uncomment this catch block and the #include of "Poco/Exception.h" if using Poco.
        //catch (Poco::Exception &ex)
        //{
        //    std::ostringstream msg;
        //    msg << msgPrefix.str() << "Poco::Exception: " << ex.displayText();
        //    LOGERROR(msg.str());
        //    return TskModule::FAIL;
        //}
        catch (std::exception &ex)
        {
            std::ostringstream msg;
            msg << msgPrefix.str() << "std::exception: " << ex.what();
            LOGERROR(msg.str());
            return TskModule::FAIL;
        }
        // Uncomment this catch block and add necessary .NET references if using C++/CLI.
        //catch (System::Exception ^ex)
        //{
        //    std::ostringstream msg;
        //    msg << msgPrefix.str() << "System::Exception: " << Maytag::systemStringToStdString(ex->Message);
        //    LOGERROR(msg.str());
        //    return TskModule::FAIL;
        //}
        catch (...)
        {
            LOGERROR(msgPrefix.str() + "unrecognized exception");
            return TskModule::FAIL;
        }
    }

    /**
     * Module execution function for post-processing modules.
     *
     * CAVEAT: This function is intended to be called by TSK Framework only.
     * Linux/OS-X modules should *not* call this function within the module
     * unless appropriate compiler/linker options are used to bind all
     * library-internal symbols at link time.
     *
     * @returns TskModule::OK on success, TskModule::FAIL on error
     */
    TskModule::Status TSK_MODULE_EXPORT report()
    {
        // The TSK Framework convention is to prefix error messages with the
        // name of the module/class and the function that emitted the message.
        std::ostringstream msgPrefix;
        msgPrefix << MODULE_NAME << "::report : ";

        // Well-behaved modules should catch and log all possible exceptions
        // and return an appropriate TskModule::Status to the TSK Framework.
        try
        {
            // If this module could be used in a post-processing pipeline, the
            // code would go here.
            
            std::cout << "##INODE-CARVER-BEGIN##" << std::endl;
            
            InodeFileReader m_FileReader;
            
            size_t blockSize = m_FileReader.getBlockSize();
            InodeCarver iCarve;
            bool check = iCarve.setArgument(m_Argument, m_FileReader);
            if (check == false)
            {
                std::cerr << "Have to abort." << std::endl;
                return TskModule::FAIL;
            }
            int err = 0;
            size_t count = 0;
//            size_t temp = 0;
            
            std::cout << "Searching for inodes." << std::endl;
            std::cout << "All necessary parameters are the default mkfs values or are taken from the config file for this module. It is located at:" << std::endl;
            std::cout << "sleuthkit-x.x/framework/runtime/modules_config/InodeModule_arguments.txt" << std::endl;
            // Iterate over the whole image to find inodes
            while (true)
            {
                err = m_FileReader.readNewBlock();
                if (err < (int)blockSize)
                {
                    blockSize = err;
//                    temp += blockSize;
                    //std::cout << "Block read: " << blockSize << std::endl;
                    iCarve.searchInode(blockSize, count, m_FileReader);
                    break;
                }
                
                // Searching for an Inode
                iCarve.searchInode(blockSize, count, m_FileReader);
//                temp += blockSize;
                ++count;
            }
            //std::cout << "Read: " << temp << std::endl;
            std::cout << "Done." << std::endl;
            
            iCarve.printInodeStats();

            char answer = 'y';
            bool die = false;
            while (true)
            {
                std::cout << "Do you want recover the files with ContentData-Mode (y)? Or do you want to extract the files with the inode number and the correct file name with the MetaData-Mode (x)? You can also choose to not recover any files (n). Or do you need help with your decision (h)?" << std::endl;
                std::cout << "What do you want to do? (y/x/n/h)" << std::endl;
                
                std::cin >> answer;
                
                if (answer == 'y')
                {
                    std::cout << "Only Ext4-Blocksize has to be known." << std::endl;
                    std::cout << "Writing files." << std::endl;
                    m_FileReader.reset();
                    iCarve.writeRegFiles(m_FileReader, true);
                    std::cout << "Done." << std::endl;
                    break;
                }
                else if (answer == 'n')
                {
                    std::cout << "Nothing has to be done. Module quits here." << std::endl;
                    break;
                }
                else if (answer == 'x')
                {
                    std::cout << "All necessary Ext4-Parameters as sepcified in the config file have to be correct." << std::endl;
                    while (true)
                    {
                        std::cout << "Do you want to recover the files with the MetaData-Mode? (y/n/h)" << std::endl;
                        std::cin >> answer;
                        if (answer == 'y')
                        {
                            std::cout << "Gathering inode information." << std::endl;
                            m_FileReader.reset();
                            iCarve.gatherInodeInformation(m_FileReader);
                            std::cout << "Done." << std::endl;
                            std::cout << "Writing files." << std::endl;
                            m_FileReader.reset();
                            iCarve.writeRegFiles(m_FileReader, false);
                            std::cout << "Done." << std::endl;
                            die = true;
                            break;
                        }
                        else if (answer == 'n')
                        {
                            die = true;
                            break;
                        }
                        else if (answer == 'h')
                        {
                            std::cout << "If you just want to recover the regular files, only Ext4-blocksize is necessary." << std::endl;
                            std::cout << "If the inodes should be recovered using the directory-entries, more parameters of the file system have to be known and should be correct. If the file system wasn't built with the default values, they have to be specified in the config file." << std::endl;
                            std::cout << "Do you want to rethink your previous answer? (y/n)" << std::endl;
                            bool die2 = false;
                            while (true)
                            {
                                std::cin >> answer;
                                if (answer == 'y')
                                {
                                    die2 = true;
                                    break;
                                }
                                else if (answer == 'n')
                                {
                                    break;
                                }
                            }
                            if (die2 == true)
                                break;
                            
                        }
                        else
                        {
                            std::cout << "Another answer must be chosen. (y/n/h)" << std::endl;
                        }
                        
                    }
                    if (die == true)
                        break;
                }
                else if (answer == 'h')
                {
                    std::cout << "If you just want to recover the regular files, only Ext4-blocksize is necessary." << std::endl;
                    std::cout << "If the inodes should be recovered using the directory-entries, more parameters of the file system have to be known and should be correct. If the file system wasn't built with the default values, they have to be specified in the config file." << std::endl;
                }
                else
                {
                    std::cout << "Another answer must be chosen. (y/n/h)" << std::endl;
                }
            }
            iCarve.printInodeStats();
            
            std::cout << "##INODE-CARVER-END##" << std::endl;
            return TskModule::OK;
        }
        catch (TskException &ex)
        {
            std::ostringstream msg;
            msg << msgPrefix.str() << "TskException: " << ex.message();
            LOGERROR(msg.str());
            return TskModule::FAIL;
        }
        // Uncomment this catch block and the #include of "Poco/Exception.h" if using Poco.
        //catch (Poco::Exception &ex)
        //{
        //    std::ostringstream msg;
        //    msg << msgPrefix.str() << "Poco::Exception: " << ex.displayText();
        //    LOGERROR(msg.str());
        //    return TskModule::FAIL;
        //}
        catch (std::exception &ex)
        {
            std::ostringstream msg;
            msg << msgPrefix.str() << "std::exception: " << ex.what();
            LOGERROR(msg.str());
            return TskModule::FAIL;
        }
        // Uncomment this catch block and add necessary .NET references if using C++/CLI.
        //catch (System::Exception ^ex)
        //{
        //    std::ostringstream msg;
        //    msg << msgPrefix.str() << "System::Exception: " << Maytag::systemStringToStdString(ex->Message);
        //    LOGERROR(msg.str());
        //    return TskModule::FAIL;
        //}
        catch (...)
        {
            LOGERROR(msgPrefix.str() + "unrecognized exception");
            return TskModule::FAIL;
        }
    }

    /**
    * Module cleanup function. This is where the module should free any resources
    * allocated during initialization or execution.
    *
    * CAVEAT: This function is intended to be called by TSK Framework only.
    * Linux/OS-X modules should *not* call this function within the module
    * unless appropriate compiler/linker options are used to bind all
    * library-internal symbols at link time.
    *
    * @returns TskModule::OK on success and TskModule::FAIL on error.
    */
    TskModule::Status TSK_MODULE_EXPORT finalize()
    {
        // The TSK Framework convention is to prefix error messages with the
        // name of the module/class and the function that emitted the message.
        std::ostringstream msgPrefix;
        msgPrefix << MODULE_NAME << "::finalize : ";

        // Well-behaved modules should catch and log all possible exceptions
        // and return an appropriate TskModule::Status to the TSK Framework.
        try
        {
            // If this module required finalization, the finalization code would
            // go here.

            return TskModule::OK;
        }
        catch (TskException &ex)
        {
            std::ostringstream msg;
            msg << msgPrefix.str() << "TskException: " << ex.message();
            LOGERROR(msg.str());
            return TskModule::FAIL;
        }
        // Uncomment this catch block and the #include of "Poco/Exception.h" if using Poco.
        //catch (Poco::Exception &ex)
        //{
        //    std::ostringstream msg;
        //    msg << msgPrefix.str() << "Poco::Exception: " << ex.displayText();
        //    LOGERROR(msg.str());
        //    return TskModule::FAIL;
        //}
        catch (std::exception &ex)
        {
            std::ostringstream msg;
            msg << msgPrefix.str() << "std::exception: " << ex.what();
            LOGERROR(msg.str());
            return TskModule::FAIL;
        }
        // Uncomment this catch block and add necessary .NET references if using C++/CLI.
        //catch (System::Exception ^ex)
        //{
        //    std::ostringstream msg;
        //    msg << msgPrefix.str() << "System::Exception: " << Maytag::systemStringToStdString(ex->Message);
        //    LOGERROR(msg.str());
        //    return TskModule::FAIL;
        //}
        catch (...)
        {
            LOGERROR(msgPrefix.str() + "unrecognized exception");
            return TskModule::FAIL;
        }
    }
}
