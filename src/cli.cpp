#include "cli.h"


CLIMode::CLIMode(string libPath){

    localLibrary = new Library();
    setLocalLibrary(libPath);

}

CLIMode::~CLIMode(){
    if(localLibrary != nullptr){
        if(localLibrary->serverActive){
            stopServer();
        }
    }
}

int CLIMode::run(){
    int retVal = 0;
    if(serverMode){
        cout << "Operating in Server mode!" << endl;
        retVal = serverRun();
    }else{
        cout << "Operating in Client mode!" << endl;
        retVal = clientRun();
    }
    return retVal;
}

int CLIMode::clientRun(){
    int retVal = 0;
    string inp;
    int action = 0;
    do{
        cout << "Action to run: 0 - exit, 1 - sync with a remote server(default), 2 - display local library contents" << endl;
        while(getline(cin, inp)){
            if(!inp.empty()){
                stringstream inpStream(inp);
                inpStream >> action;
            }else{
                action = 1;
            }
            if(action == 0 || action == 1 || action == 2) break;
            cout << "Invalid input. Please enter a valid integer: ";
            cin.clear(); 
        }

        
        if(action == 1){
            cout << "Enter the address of the remote server:" << endl;
            while(getline(cin, inp)){

                if(!inp.empty()) break;

                cout << "Invalid input. Please enter a valid address: ";
                cin.clear();  
            }

            remoteAddr = inp;
            

            if(initConn(&localLibrary->remoteLibJson,  remoteAddr)){
                cout << "Failed connecting to the server at the provided address!" << endl;
                retVal = 1;
            }
            Library diffLib;
            if(localLibrary->generateDiff(diffLib) == 1){
                cout << "Failed synchronizing data with the server!" << endl;
                retVal = 1;
                continue;
            }

            if(diffLib.artistList.empty()){
                cout << "The local library is already up to date!" << endl;
                continue;
            }


            if(localLibrary->implementDiff(diffLib) == 1){
                cout << "Failed implementing the server sync data!" << endl;
                return 1;
            }
            cout << "Synchronization with the server finished" << endl;
            

        }else if(action == 2){
            localLibrary->displayData();
            cin.clear();

        }
    }while(action > 0);

    return retVal;
}

int CLIMode::serverRun(){
    int retVal = 0;
    string inp;
    int action = 0;
    
    do{
        cout << "Action to run: 0 - exit, 1 - initialize server(default), 2 - display local library contents" << endl;
        while(getline(cin, inp)){
            if(!inp.empty()){
                stringstream inpStream(inp);
                inpStream >> action;
            }else{
                action = 1;
            }
            if(action == 0 || action == 1 || action == 2) break;
            cout << "Invalid input. Please enter a valid integer: ";
            cin.clear(); 
        }

        if(action == 1){
            localLibrary->serverActive = true;
            // run server on diff thread, cin for user input to stop the server
            retVal = initConn(&localLibrary->libJson);
            if(retVal){
                cout << "Error operating the server!" << endl;
                return retVal;
            }
            localLibrary->serverActive = false;
        }else if(action == 2){
            localLibrary->displayData();
        }
    }while(action > 0);

    return retVal;
}

int CLIMode::setLocalLibrary(string libPath){
    if(!libPath.empty()){

        if(filesystem::exists(libPath)){
            if(localLibrary->buildLibrary(libPath)){
                cout << "Error initializing Library from provided path";
                return 1;
            }else{
                libSet = true;
            }
        }else{
            cout << "Error initializing Library from provided path" << endl;
            return 1;
        }

    }else{
        if(localLibrary->jsonRead()){
            cout << "Error reading JSON library file" << endl;
            return 1;
        }else{
            libSet = true;
        }
    }

    return 0;
}