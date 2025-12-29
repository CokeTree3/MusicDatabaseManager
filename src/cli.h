#pragma once

#include "sys_headers.h"
#include "library.h"
#include "networking.h"


class CLIMode{
    

    public:
        Library* localLibrary;
        bool initDone = false;

        CLIMode(string libPath = "");
        ~CLIMode();

        int run();
        int setLocalLibrary(string libPath);

    private:
        int clientRun();
        int serverRun();

};
