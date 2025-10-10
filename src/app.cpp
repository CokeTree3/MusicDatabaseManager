#include "app.h"
#include "library.h"
#include "qobject.h"


void printTree(string searchDir){
    int prevDepth = 0;
    for (auto i = filesystem::recursive_directory_iterator(searchDir); i != filesystem::recursive_directory_iterator(); ++i){
        if(prevDepth < i.depth()){
            cout << string(prevDepth << 1, ' ') << " \\ " "\n";
        }
        
        cout << i.depth();
        cout << string(i.depth() << 1, ' ') << "|--" << i->path().lexically_relative(searchDir).string() << "\n";   // \u2015 or -- 
        prevDepth = i.depth();
    }

}


int main(int argc, char *argv[])
{

    QApplication app(argc, argv);

    WindowGUI win;
    Library library;

    win.setLocalLibrary(&library);
    if(argc > 1 && filesystem::exists(argv[1])){
        library.buildLibrary(argv[1]);
        win.setMainWindowContent(&library);
    }else{
        if(library.jsonRead()){
            cout << "Lib loading not done\n";
            
        }else{
            win.setMainWindowContent(&library);
        }
    }


    win.show();
    cout << qVersion() << endl;
    
    //library.displayData();

    return app.exec();
}
