#include "helper_func.h"

uint UcharArrayToUintLE(const unsigned char bytes[4]){
    return (uint)((bytes[0]) | ((bytes[1]) << 8) | ((bytes[2]) << 16) | ((bytes[3]) << 24));
}

bool namePathCheck(const string& name){
    if(name == "" || name == "." || name == ".."){
        return false;
    }

    filesystem::path fsPath(name);

    if(fsPath.empty()){
        return false;
    }

    if(distance(fsPath.begin(), fsPath.end()) != 1){
        return false;
    }
    return true;
}