# App Tools

This tool installs an application from multiple AppVars.

Substitute the below macros in the makefile based on how the AppVars were created.
Note that the length of `APPVAR_PREFIX` should allow for the AppVar index to be appended to the name.

    APPVAR_PREFIX = "APP"
    APPVAR_SPLIT_SIZE = 65200

You may copy the sources of this repository to make your own installer as long as you know what you are doing.

You could use a command similar to this one to output an application built with the CE C/C++ Toolchain:

    convbin --iformat 8ek --input bin/PRGM.8ek --oformat 8xv-split --maxvarsize 65200 --output bin/PRGM.8xv --name PRGM
