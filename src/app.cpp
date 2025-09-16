#include "sys_headers.h"
#include "library.h"


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


int directoryToLibrary(string searchDir, Library* library){
    if(searchDir == "" || library == nullptr){
        return 1;
    }

    for (auto i = filesystem::directory_iterator(searchDir); i != filesystem::directory_iterator(); ++i){
        if(!i->is_directory()){
            continue;
        }
        
        //cout << i->path().lexically_relative(searchDir).string() << endl;
        Artist newArtist(i->path().lexically_relative(searchDir).string(), i->path().string());

        //newArtist.printData();
        library->addToLibrary(newArtist);

    }

    return 0;
}


int main(int argc, char *argv[])
{
    string searchDir = filesystem::current_path().string();
    if(argc > 1 && filesystem::exists(argv[1])){
        searchDir = argv[1];
    }

    Library library;
    
    directoryToLibrary(searchDir, &library);
    cout << endl;
    library.printData();
    return 0;
}
