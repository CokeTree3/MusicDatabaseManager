#include "app.h"

bool libSet = false;
bool serverMode = false;
string remoteAddr = "192.168.000.000";


int main(int argc, char *argv[])
{
    libSet = false;
    serverMode = false;

    QApplication app(argc, argv);
    
    WindowGUI win;
    Library library;

    win.setLocalLibrary(&library);
    if(argc > 1 && filesystem::exists(argv[1])){
        libSet = true;
        if(library.buildLibrary(argv[1])){
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
