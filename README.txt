Inode Carver Module
Sleuth Kit Framework C++ Module
May 2015


This module is for the C++ Sleuth Kit Framework.


DESCRIPTION

This module is a file carving module that performs a 
search for inode entries of files on the system. The inodes
point to data blocks with the content of the file which can
then be recovered.

DEPLOYMENT REQUIREMENTS

This module does not have any specific deployment requirements.

USAGE

Add this module to a post processing pipeline.  See the TSK 
Framework documents for information on adding the module 
to the pipeline:

    http://www.sleuthkit.org/sleuthkit/docs/framework-docs/

This module takes configuration arguments.
The arguments are deployed by a text file located in:
framework/runtime/modules_config/InodeModule_arguments.txt
There is also a description

RESULTS

The result of the module is written to a folder
specified in the configuration file.
