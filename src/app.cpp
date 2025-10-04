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

    string searchDir = filesystem::current_path().string();
    if(argc > 1 && filesystem::exists(argv[1])){
        searchDir = argv[1];
    }

    Library library;
    library.buildLibrary(searchDir);

    WindowGUI win;
    win.setmainWindowContent(&library);
    win.show();
    cout << qVersion() << endl;
    
    //library.displayData();

    return app.exec();
}
