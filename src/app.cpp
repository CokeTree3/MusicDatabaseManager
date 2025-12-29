#include "app.h"

bool libSet = false;
bool serverMode = false;
string remoteAddr = "192.168.000.000";

#if defined (CLI_ONLY)

int exec(int argc, char *argv[], string libPath){
    (void)argc;
    (void)argv;
    cout << "Select operation mode\n (1 - server mode(default), 2 - client mode):\n";
    string inp;
    int cliOpMode = 0;

    while(getline(cin, inp)){
        if(!inp.empty()){
            stringstream inpStream(inp);
            inpStream >> cliOpMode;
        }else{
            cliOpMode = 1;
        }
        if(cliOpMode == 1 || cliOpMode == 2) break;
        cout << "Invalid input. Please enter a valid integer: ";
        cin.clear(); 
    }

    if(cliOpMode == 1){
        cout << "server ops" << endl;       // open server on 2nd thread, listen for requests
        serverMode = true;
    }else if(cliOpMode == 2){
        cout << "client ops" << libPath << endl;       // ask for ip, init conn to the ip, do sync
    }else{
        return 1;
    }

    CLIMode app(libPath);

    return app.run();
}

#else

int exec(int argc, char *argv[], string libPath){
    libSet = false;

    QApplication app(argc, argv);
    WindowGUI win;
    Library library;

    win.setLocalLibrary(&library);
    if(argc > 1 && filesystem::exists(libPath)){
        libSet = true;
        if(library.buildLibrary(libPath)){
            win.showErrorPopup(1,"Error initializing Library from provided path");
            library.resetLibrary();
        }else{
            libSet = true;
            win.setMainWindowContent(&library);
        }
    }else{
        if(library.jsonRead()){
            cout << "Lib loading not done\n";
            win.showErrorPopup(1,"Error reading JSON library file");
        
        }else if(libSet){
            win.setMainWindowContent(&library);
        }
    }


    win.show();

    return app.exec();
}

#endif


int main(int argc, char *argv[])
{
    serverMode = false;
    string libPath = "";
    int mode = 0;
    int retVal;

    if(argc > 1){
        for(int i = 1; i < argc; i++){
    
            if(argv[i][0] == '-'){
                
                if(!strcmp(argv[i], "--no-gui")){
                    mode = 1;
                }else if(!strcmp(argv[i], "-l")){
                    if(i < argc - 1 && argv[i+1][0] != '-'){
                        libPath = string(argv[i+1]);
                        i++;
                    }else{
                        cout << "Incorrect Option Format" << endl;
                        return 1;
                    }
                }else if(!strcmp(argv[i], "-h") || !strcmp(argv[i], "--help")){
                    cout << "TODO" << endl;
                }
                /*else if(!strcmp(argv[i], "-s") || !strcmp(argv[i], "--server")){
                    mode = 1;
                }else if(!strcmp(argv[i], "-c") || !strcmp(argv[i], "--client")){
                    mode = 2;
                }*/else{
                    cout << "Unknown Option \"" << argv[i] << "\"" << endl;
                    return 1;
                }
            }else{
                cout << "Incorrect Options" << endl;
                return 1;
            }
        }
    }
    retVal = exec(argc, argv, libPath);

    return retVal;
}
