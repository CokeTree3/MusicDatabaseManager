#include "library.h"

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

#if defined (PLATFORM_LINUX) || defined (PLATFORM_WINDOWS)
int createNewFile(string path, string fName, string libPath){

    filesystem::path fPath;

#if defined (PLATFORM_WINDOWS)
    fPath = filesystem::u8path(path + fName);
#elif defined (PLATFORM_LINUX)
    fPath = filesystem::path(path + fName);
#endif

    ofstream trackFile(fPath, ios::binary);

    if(!trackFile.is_open()){
        cout << "error creating a new file: " << path + fName << endl;
        return 1;
    }
    string relativePath = string(path+fName).substr(libPath.length());

    vector<char> buf;

    requestTrack(relativePath, buf);

    if(buf.empty()){
        cout << "Error requesting server file: " << relativePath << endl;
        return 1;
    }
    trackFile.write(buf.data(), buf.size());

    trackFile.close();
    return 0;
}
#endif

#if defined (PLATFORM_ANDROID)
int createNewFile(string path, string fName, string libPath){
    string relativePath = path.substr(libPath.length() + 1);

    //if(!permsObtained){                                           // permsObtained is a Library element, this func isnt part of Library anymore, needs a fix
        requestPermissions();
    //}
    vector<char> buf;
    string relativePathWName = string(path+fName).substr(libPath.length());
    requestTrack(relativePathWName, buf);

    if(buf.empty()){
        cout << "Error requesting server file: " << relativePath << endl;
        return 1;
    }

    int res = createFileAndroid(fName, relativePath, buf.data(), buf.size());

    if(res != 0){
        cout << "Error creating track file: " << path + fName <<  endl;
        return 1;
    }
    return 0;
}
#endif

void removeDir(string absolutePath){

#if defined (PLATFORM_LINUX) || defined (PLATFORM_WINDOWS)
    error_code ec;
    filesystem::remove_all(absolutePath, ec);
    if(ec){
        cout << "ERROR: couldn't remove directory or file: " << absolutePath << "\n Error: " << ec.message() << endl;
    }
#elif defined (PLATFORM_ANDROID)                                          // Dir removing on Android not implemented
    cout << "Directory removing is not supported(" << absolutePath << ")" << endl;
#endif
}

void removeFile(string absolutePath){

#if defined (PLATFORM_LINUX) || defined (PLATFORM_WINDOWS)
    error_code ec;
    filesystem::remove(absolutePath, ec);
    if(ec){
        cout << "ERROR: couldn't remove file: " << absolutePath << "\n Error: " << ec.message() << endl;
    }
#elif defined (PLATFORM_ANDROID)                                          // File removing on Android not implemented
    QFile fileToRemove(QString::fromStdString(absolutePath));
    if(fileToRemove.exists()){
        fileToRemove.remove();
    }
#endif
    return;
}

void makeDir(string absolutePath){                                          // Directory creating on Android not needed

#if defined (PLATFORM_LINUX) || defined (PLATFORM_WINDOWS)
    error_code ec;
    filesystem::create_directory(absolutePath, ec);
    if(ec){
        cout << "ERROR: couldn't create directory: " << absolutePath << "\n Error: " << ec.message() << endl;
    }
#endif
    return;
}

/*
*   Library Class Functions
*/
int Library::buildLibrary(string searchDir){
    if(searchDir == "" || !filesystem::exists(searchDir)){
        cout << "Incorrect library path!" << endl;
        return 1;
    }
    if(serverActive){
        cout << "This library is currently used by an active server thread!/n Action not permitted!" << endl;
        return 1;
    }
    cout << "building from " << searchDir << endl;
    libPath = searchDir;

#if defined (PLATFORM_LINUX) || defined (PLATFORM_WINDOWS)
    for (auto i = filesystem::directory_iterator(searchDir); i != filesystem::directory_iterator(); ++i){
        if(!i->is_directory()){
            continue;
        }
        this->addToLibrary(i->path().lexically_relative(searchDir).string(), i->path().string());
    }
#elif defined (PLATFORM_ANDROID)

    QDirIterator iter(QString::fromStdString(searchDir), QDir::NoDotAndDotDot | QDir::AllEntries);
    while(iter.hasNext()){
        QString path = iter.next();
        if(!iter.fileInfo().isDir()){
            continue;
        }
        if(this->addToLibrary(iter.fileName().toStdString(), iter.filePath().toStdString())) return 1;
    }
#endif

    if(jsonBuild()){
        cout << "Error writing to local JSON file" << endl;
    }
    return 0;
}

int Library::jsonBuild(){
    if(serverActive){
        cout << "This library is currently used by an active server thread!/n Action not permitted!" << endl;
        return 1;
    }

    cout << "call to build" << endl;

    ofstream s("library.json", ios::binary);

    if(!s.is_open()){
        return 1;
    }

    size_t count = artistList.size();
    cout << libPath << endl;
    libJson["LibraryPath"] = libPath;
    libJson["ArtistCount"] = count;
    libJson["Artists"] = json::array({});
    for(size_t i = 0; i < count; i++){
        libJson["Artists"].push_back(artistList[i]->getJsonStructure());
    }

    s << setw(4) << libJson;
    s.close();

    return 0;
}

int Library::jsonRead(){
    if(serverActive){
        cout << "This library is currently used by an active server thread!/n Action not permitted!" << endl;
        return 1;
    }
    string libFile = "library.json";

    if(access( libFile.c_str(), F_OK ) != -1){
        ifstream f;
        json jsonData;
        try{
            f.open(libFile, ios::binary);
            jsonData = json::parse(f);
            f.close();
        }catch(...){
            if(f.is_open()){
                f.close();
            }
            return 1;
        }
        if(buildLibrary(jsonData["LibraryPath"])){
            libSet = false;
            return 1;
        }
        libSet = true;
        return 0;

    }else{
        cout << "Creating a new Library JSON file\n";
        ofstream of(libFile, ios::binary);
        if(!of){
            return 1;
        }
        libJson["LibraryPath"] = libPath;
        of << setw(4) << libJson;
        of.close();
        return 0;
    }
    
}

int Library::buildFromJson(json jsonSource){
    if(serverActive){
        cout << "This library is currently used by an active server thread!/n Action not permitted!" << endl;
        return 1;
    }
    if(!jsonSource.contains("Artists")) return 1;
    this->libJson = jsonSource;
    this->libPath = "";
    if(jsonSource.contains("test")) cout << jsonSource["test"] << endl;
    cout << jsonSource["test"] << endl;
    for(size_t i = 0; i < jsonSource["Artists"].size(); i++){
        this->addToLibrary(jsonSource["Artists"][i]);
    }

    return 0;
}

int Library::findDiff(Library* remoteLib, json* jsonDiff){
    (*jsonDiff)["Artists"] = json::array({});
    vector<bool> markList(remoteLib->artistList.size(), false);
    for(size_t i = 0; i < this->artistList.size(); i++){
        bool found = false;
        json artistDiff;
        for(size_t j = 0; j < remoteLib->artistList.size(); j++){
            if(this->artistList[i]->equals(remoteLib->artistList[j], &artistDiff) && !markList[j]){
                markList[j] = true;
                found = true;
                break;
            }
        }
        if(!found){
            (*jsonDiff)["Artists"].push_back(this->artistList[i]->getJsonStructure());

        }
        else if(found && !artistDiff.is_null()){
            (*jsonDiff)["Artists"].push_back(artistDiff);
        }
    }
    for(size_t i = 0; i < markList.size(); i++){
        if(!markList[i]){
            json artistToRemove;
            artistToRemove["1Name"] = remoteLib->artistList[i]->name;
            artistToRemove["Remove"] = true;
            artistToRemove["Albums"] = json::array({});

            (*jsonDiff)["Artists"].push_back(artistToRemove);
        }
    }

    return 0;
}

int Library::generateDiff(Library &diffLib){
    if(remoteLibJson.empty()){
        cout << "Could not obtain server library file!\nRetry connection\n";
        return 1;
    }

    json diff;
    Library serverLib;

    if(serverLib.buildFromJson(remoteLibJson)) return 1;
    if(serverLib.findDiff(this, &diff)) return 1;

    if(diff.empty()){
        return 1;
    }

    /*ofstream f("libDiff.json", ios::binary);
    f << setw(4) << diff;
    f.close();*/

    return diffLib.buildFromJson(diff);
}

int Library::syncWithServer(){
    Library diffLib;
    if(generateDiff(diffLib)){
        return 1;
    }
    return implementDiff(diffLib);
}

int Library::implementDiff(Library &diffLib){

    cout << "starting fs operations\n";
    for(size_t i = 0; i < diffLib.artistList.size(); i++){
        if(diffLib.artistList[i]->implementDiff(*this, libPath, libPath)) return 1;
    }
    
    return 0;
}

int Library::addToLibrary(string name, string dirPath){
    artistList.push_back(make_unique<Artist>(name, dirPath));
    return artistList.size() - 1;
}

int Library::addToLibrary(json jsonSource){
    artistList.push_back(make_unique<Artist>(jsonSource));
    return artistList.size() - 1;
}

int Library::addToLibrary(Artist& toCopyFrom, string fullPath, string libPath){
    artistList.push_back(make_unique<Artist>(toCopyFrom, fullPath, libPath));
    return artistList.size() - 1;
}

int Library::removeFromLibrary(string name, int indexInList){
    if(this->artistList[indexInList]->name != name){
        return -1;
    }
    this->artistList.erase(this->artistList.begin() + indexInList);
    return artistList.size();
}

void Library::resetLibrary(){
    artistList.clear();
    libJson.clear();
}

void Library::printData(){
    cout << "Artists in the database: \n";
    int count = 1;
    for(size_t i = 0; i < artistList.size(); i++){
        cout << count << ".  " << artistList[i]->name << endl;
        count++;
    }
}

void Library::displayData(){
    this->printData();

    if(artistList.size() > 0){
        cout << "Select which artist to display: ";
        int nr = 0;
        while (!(cin >> nr) || nr < 1 || nr > (int)artistList.size()) {
            cout << "Invalid input. Please enter a valid integer: ";
            cin.clear(); 
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
    
        artistList[nr-1]->displayData();
    }
}



/*
*   Artist Class Functions
*/
Artist::Artist(string name){
    if(namePathCheck(name)){
        this->name = name;
    }else{
        this->name = "";
        cout << "Artist name contains invalid characters" << endl;
    }

    albumCount = 0;
}

Artist::Artist(string name, string dirPath){
    if(namePathCheck(name)){
        this->name = name;
    }else{
        this->name = "";
        cout << "Artist name contains invalid characters" << endl;
    }

    this->albumCount = 0;
    if(directoryToAlbums(dirPath)){
        albumCount = 0;
        albumList.clear();
        cout << "Error creating Library, Failed generating album list of: " << this->name << endl;
    }
}

Artist::Artist(json jsonSource){
    if(namePathCheck(jsonSource["1Name"])){
        this->name = jsonSource["1Name"];
    }else{
        this->name = "";
        cout << "Artist name contains invalid characters" << endl;
    }

    this->albumCount = 0;
    if(jsonSource.contains("Remove") && jsonSource["Remove"] == true){
        this->toBeRemoved = true;
        return;
    }
    for(size_t i = 0; i < jsonSource["Albums"].size(); i++){
        this->addAlbum(jsonSource["Albums"][i]);
    }
}

Artist::Artist(Artist& toCopyFrom, string fullPath, string libPath){
    this->name = toCopyFrom.name;
    this->albumCount = 0;
    if(fullPath == "" || libPath == ""){
        for(size_t i = 0; i < toCopyFrom.albumList.size(); i++){
            this->addAlbum(*toCopyFrom.albumList[i]);
        }
    }else{
        for(size_t i = 0; i < toCopyFrom.albumList.size(); i++){
            if(toCopyFrom.albumList[i]->implementDiff(*this, fullPath, libPath)){
                cout << "Error creating Library, Failed generating album list of: " << this->name << endl;
            }
        }
    }
    
}


void Artist::printData(){
    cout << name << " albums - \n";
    for(size_t i = 0; i < albumCount; i++){
        cout << i + 1 << ". " << albumList[i]->name << endl;
    }
}

void Artist::displayData(){
    this->printData();

    if(albumList.size() > 0){
        cout << "Select which album to display: ";
        int nr = 0;
        while (!(cin >> nr) || nr < 1 || nr > (int)albumList.size()) {
            cout << "Invalid input. Please enter a valid integer: ";
            cin.clear(); 
            cin.ignore(numeric_limits<streamsize>::max(), '\n');
        }
        albumList[nr-1]->printData();
    }
}

bool Artist::equals(unique_ptr<Artist>& rhs, json* jsonDiff){
    if(this->name == rhs->name){
        bool init = false;
        vector<bool> markList(rhs->albumList.size(), false);
        for(size_t i = 0; i < this->albumList.size(); i++){
            bool found = false;
            json albumDiff;
            for(size_t j = 0; j < rhs->albumList.size(); j++){
                if(this->albumList[i]->equals(rhs->albumList[j], &albumDiff) && !markList[j]){
                    markList[j] = true;
                    found = true;
                    break;
                }
            }
            if(!found){
                if(!init){
                    init = true;
                    (*jsonDiff) = this->getEmptyJsonStructure();
                }
                (*jsonDiff)["Albums"].push_back(this->albumList[i]->getJsonStructure());
                
            }
            else if(found && !albumDiff.is_null()){
                if(!init){
                    init = true;
                    (*jsonDiff) = this->getEmptyJsonStructure();
                }
                (*jsonDiff)["Albums"].push_back(albumDiff);
            }
        }
        for(size_t i = 0; i < markList.size(); i++){
            if(!markList[i]){
                if(!init){
                    init = true;
                    (*jsonDiff) = this->getEmptyJsonStructure();
                }
                json albumToRemove = rhs->albumList[i]->getEmptyJsonStructure();
                albumToRemove["Remove"] = true;


                (*jsonDiff)["Albums"].push_back(albumToRemove);

            }
        }
        return true;
    }
    return false;
}

int Artist::implementDiff(Library& mainLib, string rootPath, const string libPath){
    string artistPathFull = rootPath + "/" + this->name;

    cout << artistPathFull << endl;
    if(this->toBeRemoved){
        removeDir(artistPathFull);
        for(size_t i = 0; i < mainLib.artistList.size(); i++){
            if(this->name == mainLib.artistList[i]->name){
                if(mainLib.removeFromLibrary(this->name, i)) return 1;
                break;
            }
        }
        return  0;
    }
    bool found = false;
    for(size_t i = 0; i < mainLib.artistList.size(); i++){
        if(this->name == mainLib.artistList[i]->name){
            found = true;
            for(size_t j = 0; j < this->albumList.size(); j++){
                if(this->albumList[j]->implementDiff(*mainLib.artistList[i], artistPathFull, libPath)){
                    cout << "Error creating Library, Failed generating album list of: " << this->name << endl;
                }
            }
            break;
        }
    }
    if(!found){
        makeDir(artistPathFull);
        mainLib.addToLibrary(*this, artistPathFull, libPath);   
    }
    return 0;
}

json Artist::getJsonStructure(){
    json data;
    data["1Name"] = this->name;
    data["AlbumCount"] = this->albumCount;
    data["Albums"] = json::array({});

    for(size_t i = 0; i < albumCount; i++){
        data["Albums"].push_back(albumList[i]->getJsonStructure());
    }

    return data;
}

json Artist::getEmptyJsonStructure(){
    json data;
    data["1Name"] = this->name;
    data["Albums"] = json::array({});
    
    return data;
}

int Artist::addAlbum(string name, string dirPath){
    albumList.push_back(make_unique<Album>(name, dirPath));
    albumCount++;
    return albumCount - 1;
}

int Artist::addAlbum(json jsonSource){
    albumList.push_back(make_unique<Album>(jsonSource));
    albumCount++;
    return albumCount - 1;
}

int Artist::addAlbum(Album& toCopyFrom, string fullPath, string libPath){
    albumList.push_back(make_unique<Album>(toCopyFrom, fullPath, libPath));
    albumCount++;
    return albumCount - 1;
}

int Artist::removeAlbum(string name, int indexInList){
    if(this->albumList[indexInList]->name != name){
        return 1;
    }
    this->albumList.erase(this->albumList.begin() + indexInList);
    return 0;
}

int Artist::directoryToAlbums(string path){
    if(path == ""){
        return 1;
    }
#if defined (PLATFORM_LINUX) || defined (PLATFORM_WINDOWS)

    for (auto i = filesystem::directory_iterator(path); i != filesystem::directory_iterator(); ++i){
        if(!i->is_directory()){
            continue;
        }
        addAlbum(i->path().lexically_relative(path).string(), i->path().string());
    }

#elif defined (PLATFORM_ANDROID)
    
    QDirIterator iter(QString::fromStdString(path), QDir::NoDotAndDotDot | QDir::AllEntries);
    while(iter.hasNext()){
        QString path = iter.next();
        if(!iter.fileInfo().isDir()){
            continue;
        }
        addAlbum(iter.fileName().toStdString(), iter.filePath().toStdString());
    }
#endif
    return 0;
}



/*
*   Album Class Functions
*/
Album::Album(string name){
    if(namePathCheck(name)){
        this->name = name;
    }else{
        this->name = "";
        cout << "Album name contains invalid characters" << endl;
    }

#if defined (PLATFORM_LINUX) || defined (PLATFORM_WINDOWS)
    this->coverPath = filesystem::current_path().string() + "/assets/missingCov.jpg";

#elif defined (PLATFORM_ANDROID)
    this->coverPath = QDir::currentPath().toStdString() + "/assets/missingCov.jpg";
#endif
    trackCount = 0;
}

Album::Album(string name, string dirPath){
    if(namePathCheck(name)){
        this->name = name;
    }else{
        this->name = "";
        cout << "Album name contains invalid characters" << endl;
    }
    
    this->trackCount = 0;
    if(directoryToTracks(dirPath)){
        trackCount = 0;
        trackList.clear();
        cout << "Error creating Library, Failed generating track list of album: " << this->name << ", path: " << dirPath << endl;
    }

#if defined (PLATFORM_LINUX) || defined (PLATFORM_WINDOWS)                                  // Add path searching for cover photo(as a helper func)

    if(filesystem::exists(filesystem::path(dirPath + "/cover.jpg"))){
        coverPath = dirPath + "/cover.jpg";
    }
    else if(filesystem::exists(filesystem::path(dirPath + "/cover.png"))){
        coverPath = dirPath + "/cover.png";
    }else{
        coverPath = filesystem::current_path().string() + "/assets/missingCov.jpg";
    }

#elif defined (PLATFORM_ANDROID)

    if(QFileInfo::exists(QString::fromStdString(dirPath + "/cover.jpg"))){
        coverPath = dirPath + "/cover.jpg";
    }
    else if(QFileInfo::exists(QString::fromStdString(dirPath + "/cover.png"))){
        coverPath = dirPath + "/cover.png";
    }else{
        coverPath = QDir::currentPath().toStdString() + "/assets/missingCov.jpg";
    }

#endif
}

Album::Album(json jsonSource){
    if(namePathCheck(jsonSource["1Name"])){
        this->name = jsonSource["1Name"];
    }else{
        this->name = "";
        cout << "Album name contains invalid characters" << endl;
    }

    this->trackCount = 0;
    if(jsonSource.contains("Remove") && jsonSource["Remove"] == true){
        this->toBeRemoved = true;
        return;
    }

    for(size_t i = 0; i < jsonSource["Tracks"].size(); i++){
        addTrack(jsonSource["Tracks"][i]);
    }
    this->coverPath = "";
}

Album::Album(Album& toCopyFrom, string fullPath, string libPath){
    this->name = toCopyFrom.name;
    this->trackCount = 0;
    this->coverPath = "";                                       // Add path searching for cover photo(as a helper func)
    if(fullPath == "" || libPath == ""){
        for(size_t i = 0; i < toCopyFrom.trackList.size(); i++){
            this->addTrack(*toCopyFrom.trackList[i]);
        }
    }else{
        for(size_t i = 0; i < toCopyFrom.trackList.size(); i++){
            if(toCopyFrom.trackList[i]->implementDiff(*this, fullPath, libPath)){
                cout << "Error creating Library, Failed generating track list of: " << this->name << ", path: " << fullPath << endl;
            }
        }
    }
}



bool Album::equals(unique_ptr<Album>& rhs, json* jsonDiff){
    if(this->name == rhs->name){

        bool init = false;
        vector<bool> markList(rhs->trackList.size(), false);
        for(size_t i = 0; i < this->trackList.size(); i++){
            bool found = false;
            json trackDiff;
            for(size_t j = 0; j < rhs->trackList.size(); j++){
                if(this->trackList[i]->equals(rhs->trackList[j]) && !markList[j]){
                    markList[j] = true;
                    found = true;
                    break;
                }
            }
            if(!found){
                if(!init){
                    init = true;
                    (*jsonDiff) = this->getEmptyJsonStructure();
                }
                (*jsonDiff)["Tracks"].push_back(this->trackList[i]->getJsonStructure());
            }
        }
        for(size_t i = 0; i < markList.size(); i++){
            if(!markList[i]){
                if(!init){
                    init = true;
                    (*jsonDiff) = this->getEmptyJsonStructure();
                }
                json trackToRemove;
                trackToRemove["1Name"] = rhs->trackList[i]->name;
                trackToRemove["Remove"] = true;
                trackToRemove["FileName"] = rhs->trackList[i]->fileName;

                (*jsonDiff)["Tracks"].push_back(trackToRemove);

            }
        }
        return true;
    }
    return false;
}

int Album::implementDiff(Artist& mainLibArtist, string rootPath, const string libPath){
    string albumPathFull = rootPath + "/" + this->name;

    cout << "    " << albumPathFull << endl;
    if(this->toBeRemoved){
        removeDir(albumPathFull);
        for(size_t i = 0; i < mainLibArtist.albumList.size(); i++){
            if(this->name == mainLibArtist.albumList[i]->name){
                if(mainLibArtist.removeAlbum(this->name, i)) return 1;
                break;
            }
        }
        return  0;
    }
    bool found = false;
    for(size_t i = 0; i < mainLibArtist.albumList.size(); i++){
        if(this->name == mainLibArtist.albumList[i]->name){
            found = true;
            for(size_t j = 0; j < this->trackList.size(); j++){
                if(this->trackList[j]->implementDiff(*mainLibArtist.albumList[i], albumPathFull, libPath)){
                    cout << "Error creating Library, Failed generating track list of: " << this->name << ", path: " << rootPath << endl;
                    return 1;
                }
            }
            break;
        }
    }
    if(!found){
        makeDir(albumPathFull);
        mainLibArtist.addAlbum(*this, albumPathFull, libPath);
    }
    return 0;
}


void Album::printData(){
    cout << name << " with "<< trackCount << " Tracks: \n";
    this->printTracks();
}

void Album::printTracks(){
    for(size_t i = 0; (size_t)i < trackList.size(); i++){
        cout << i+1 << ". " << trackList[i]->name << endl;
    }
}

json Album::getJsonStructure(){
    json data;
    data["1Name"] = this->name;
    data["TrackCount"] = this->trackCount;
    data["Tracks"] = json::array({});

    for(size_t i = 0; i < trackCount; i++){
        data["Tracks"].push_back(trackList[i]->getJsonStructure());
    }

    return data;
}

json Album::getEmptyJsonStructure(){
    json data;
    data["1Name"] = this->name;
    data["Tracks"] = json::array({});

    return data;
}

int Album::addTrack(int order, string filePath){
    trackList.push_back(make_unique<Track>(order, filePath));
    trackCount++;
    return trackCount - 1;
}

int Album::addTrack(json jsonSource){
    trackList.push_back(make_unique<Track>(jsonSource));
    trackCount++;
    return trackCount - 1;
}

int Album::addTrack(Track& toCopyFrom){
    trackList.push_back(make_unique<Track>(toCopyFrom));
    trackCount++;
    return trackCount - 1;
}

int Album::removeTrack(string name, int indexInList){
    if(this->trackList[indexInList]->name != name){
        return 1;
    }
    this->trackList.erase(this->trackList.begin() + indexInList);
    return 0;
}

int Album::directoryToTracks(string path){
    if(path == ""){
        return 1;
    }

    int trackOrder = 1;

#if defined (PLATFORM_LINUX) || defined (PLATFORM_WINDOWS)

    for (auto i = filesystem::directory_iterator(path); i != filesystem::directory_iterator(); ++i){
        
        if(!i->is_regular_file()){
            continue;
        }

        string fType = i->path().extension().string();
        if(fType == ".jpg" || fType == ".png"){
            this->coverPath = i->path().string();
            continue;
        }
        if(fType == ".mp3" || fType == ".flac"){
            addTrack(trackOrder, i->path().string());
            trackOrder++;
        }
    }

#elif defined (PLATFORM_ANDROID)

    QDirIterator iter(QString::fromStdString(path), QDir::NoDotAndDotDot | QDir::AllEntries);
    while(iter.hasNext()){
        QString path = iter.next();
        if(!iter.fileInfo().isFile()){
            continue;
        }
        QString fType = iter.fileInfo().suffix();
        if(fType == "jpg" || fType == "png"){
            this->coverPath = iter.filePath().toStdString();
            continue;
        }
        if(fType == "mp3" || fType == "flac"){
            addTrack(trackOrder, iter.filePath().toStdString());
            trackOrder++;
        }
    }
#endif

    sort(trackList.begin(), trackList.end(), [](const unique_ptr<Track> &a, const unique_ptr<Track> &b) {
                  return a->orderInAlbum < b->orderInAlbum;});

    return 0;
}



/*
*   Track Class Functions
*/
Track::Track(string name, int order){
    if(namePathCheck(name)){
        this->name = name;
    }else{
        this->name = "";
        cout << "Track name contains invalid characters" << endl;
    }

    this->order = order;
    this->orderInAlbum = order;
    this->fileName = "";
}

Track::Track(int order, string filePath){
    this->order = order;
    if(readFile(filePath)){
        cout << "file reading error: " << filePath << endl;
    }
#if defined (PLATFORM_WINDOWS)
    this->fileName = filePath.substr(filePath.rfind('\\') + 1, filePath.npos);
#else
    this->fileName = filePath.substr(filePath.rfind('/') + 1, filePath.npos);
#endif
}

Track::Track(json jsonSource){
    if(namePathCheck(jsonSource["1Name"])){
        this->name = jsonSource["1Name"];
    }else{
        this->name = "";
        cout << "Track name contains invalid characters" << endl;
    }

    if(namePathCheck(jsonSource["FileName"])){
        this->fileName = jsonSource["FileName"];
    }else{
        this->fileName = "";
        cout << "Track filename contains invalid characters" << endl;
    }

    if(jsonSource.contains("Remove") && jsonSource["Remove"] == true){
        this->toBeRemoved = true;
        return;
    }
    this->orderInAlbum = jsonSource["Order"];
    this->order = this->orderInAlbum;
}

Track::Track(Track& toCopyFrom){
    this->name = toCopyFrom.name;
    this->orderInAlbum = toCopyFrom.orderInAlbum;
    this->order = toCopyFrom.order;
    this->fileName = toCopyFrom.fileName;
}

bool Track::equals(unique_ptr<Track>& rhs){
    if(this->name == rhs->name && this->orderInAlbum == rhs->orderInAlbum){
        return true;
    }
    return false;
}

json Track::getJsonStructure(){
    json data;
    try{
        if((unsigned char)this->name[0] == 0xff){
            cout << "QWQWQWQWQW " << name << order << endl;
        }
            
        data["1Name"] = this->name;
    }catch(...){
        cout << this->name << "BADBAD" << endl;
    }
    
    data["Order"] = this->orderInAlbum;
    data["FileName"] = this->fileName;

    return data;
}

int Track::implementDiff(Album& mainLibAlbum, string rootPath, const string libPath){
    if(this->fileName == "" || rootPath == "" || libPath == "") return 1;
    
    string trackPathFull = rootPath + "/" + this->fileName;

    if(this->toBeRemoved){
        cout << "removing track " << trackPathFull << endl;
        removeFile(trackPathFull);
        for(size_t i = 0; i < mainLibAlbum.trackList.size(); i++){
            if(this->name == mainLibAlbum.trackList[i]->name){
                if(mainLibAlbum.removeTrack(this->name, i)) return 1;
                break;
            }
        }
        return  0;
    }
    bool found = false;
    for(size_t i = 0; i < mainLibAlbum.trackList.size(); i++){
        if(this->name == mainLibAlbum.trackList[i]->name){
            found = true;
            cout << "This should never happen" << endl;                         // This should never happen
            break;
        }
    }
    if(!found){
        if(createNewFile(rootPath + "/", this->fileName, libPath)) return 1;
        mainLibAlbum.addTrack(*this);
    }
    return 0;
}

#if defined (PLATFORM_LINUX)// || defined (PLATFORM_WINDOWS)                                Windows currently temporarly moved to QT approach
int Track::readMP3TagFrame(ifstream& f, const string& neededTagID, string* output){

    if(!f || !f.is_open()) return 1;

    string tagID(4, '\0');
    f.read(&tagID[0], 4);

    uint frameSize;
    f.read(reinterpret_cast<char*>(&frameSize), sizeof(char) * 4);

    char* pos = (char*) &frameSize;                                             //Reversal of the Bytes after reading to get the correct value of the 'tag_size'
    for (uint i = 0; i < ((sizeof (uint)) / 2); i++){
        pos [i] = pos [i] ^ pos [(sizeof (uint)) - i - 1];
        pos [(sizeof (uint)) - i - 1] = pos [i] ^ pos [(sizeof (uint)) - i - 1];
        pos [i] = pos [i] ^ pos [(sizeof (uint)) - i - 1];
    }

    if(tagID != neededTagID){
        f.ignore(2 + frameSize);
        return readMP3TagFrame(f, neededTagID, output);
    }
    
    f.ignore(3);
    output->resize(frameSize);

    f.read(&(*output)[0], frameSize);

    return 0;
}

int Track::readFLACMetadataBlock(ifstream& f, const string& neededBlockType, string* output){

    if(!f || !f.is_open()) return 1;

    unsigned char header[4];
    f.read(reinterpret_cast<char*>(&header), sizeof(char) * 4);
    uint blockSize = (header[1] << 16) | (header[2] << 8) | header[3];
    
    if((header[0] & 0x7F) != 4){
        if(header[0] & 0x80){
            cout << "FLAC file does not contain information: " + neededBlockType << endl;
            return 1;
        }
        f.ignore(blockSize);
        
        
        return readFLACMetadataBlock(f, neededBlockType, output);
    }

    unsigned char vendorLen[4];
    f.read(reinterpret_cast<char*>(&vendorLen), sizeof(char) * 4);

    f.ignore(UcharArrayToUintLE(vendorLen));

    unsigned char vorbisFieldCount[4];
    f.read(reinterpret_cast<char*>(&vorbisFieldCount), sizeof(char) * 4);

    for(uint i = 0; i < UcharArrayToUintLE(vorbisFieldCount); i++){
        unsigned char fieldSize[4];
        f.read(reinterpret_cast<char*>(&fieldSize), sizeof(char) * 4);
        string field(UcharArrayToUintLE(fieldSize)+1, '\0');

        f.read(&field[0], UcharArrayToUintLE(fieldSize));

        if(field.find(neededBlockType + "=") == 0){
            *output = field.substr(field.find("=") + 1);
            break;
        }
    }
    return 0;

}
#endif

#if defined (PLATFORM_ANDROID)  || defined (PLATFORM_WINDOWS)
int Track::readMP3TagFrameQt(QFile& f, const string& neededTagID, string* output){

    if(!f.isOpen()) return 1;

    string tagID(4, '\0');
    f.read(&tagID[0], 4);

    uint frameSize;
    f.read(reinterpret_cast<char*>(&frameSize), sizeof(char) * 4);

    char* pos = (char*) &frameSize;                                             //Reversal of the Bytes after reading to get the correct value of the 'tag_size'
    for (uint i = 0; i < ((sizeof (uint)) / 2); i++){
        pos [i] = pos [i] ^ pos [(sizeof (uint)) - i - 1];
        pos [(sizeof (uint)) - i - 1] = pos [i] ^ pos [(sizeof (uint)) - i - 1];
        pos [i] = pos [i] ^ pos [(sizeof (uint)) - i - 1];
    }

    if(tagID != neededTagID){
        f.skip(2 + frameSize);
        
        return readMP3TagFrameQt(f, neededTagID, output);
    }
    
    
    f.skip(3);
    output->resize(frameSize);

    f.read(&(*output)[0], frameSize);

    return 0;
}

int Track::readFLACMetadataBlockQt(QFile& f, const string& neededBlockType, string* output){

    if(!f.isOpen()) return 1;

    unsigned char header[4];
    f.read(reinterpret_cast<char*>(&header), sizeof(char) * 4);

    uint blockSize = (header[1] << 16) | (header[2] << 8) | header[3];
    
    if((header[0] & 0x7F) != 4){
        if(header[0] & 0x80){
            cout << "FLAC file does not contain title information" << endl;
            return 1;
        }
        f.skip(blockSize);
        
        
        return readFLACMetadataBlockQt(f, neededBlockType, output);
    }

    unsigned char vendorLen[4];
    f.read(reinterpret_cast<char*>(&vendorLen), sizeof(char) * 4);

    f.skip(UcharArrayToUintLE(vendorLen));

    unsigned char vorbisFieldCount[4];
    f.read(reinterpret_cast<char*>(&vorbisFieldCount), sizeof(char) * 4);

    for(uint i = 0; i < UcharArrayToUintLE(vorbisFieldCount); i++){
        unsigned char fieldSize[4];
        f.read(reinterpret_cast<char*>(&fieldSize), sizeof(char) * 4);
        string field(UcharArrayToUintLE(fieldSize)+1, '\0');

        f.read(&field[0], UcharArrayToUintLE(fieldSize));

        if(field.find(neededBlockType + "=") == 0){
            *output = field.substr(field.find("=") + 1);
            break;
        }
    }
    return 0;

}
#endif


#if defined (PLATFORM_LINUX)// || defined (PLATFORM_WINDOWS)

int Track::readFile(string fileName){

    ifstream f(fileName, ios::binary);
    if(!f.is_open()){
        cout << "can't open -> " ;
        printf("%x %x %x %x ", fileName[51], fileName[52], fileName[53], fileName[54]);
        return 1;
    }
    string title = "";
    string ord = "";
    string fType = filesystem::path(fileName).extension().string();

    if(fType == ".flac"){
        char tagHeader[4];

        f.read(reinterpret_cast<char*>(&tagHeader), sizeof(char) * 4);

        if(tagHeader[0] != 0x66 && tagHeader[1] != 0x4c && tagHeader[2] != 0x61 && tagHeader[3] != 0x43){
            cout << "\n Not flac : " << fileName << endl;
            f.close();
            return 1;
        }
        f.seekg(4);

        if(readFLACMetadataBlock(f, "TITLE", &title)){
            return 1;
        }
        f.seekg(4);

        if(readFLACMetadataBlock(f, "TRACKNUMBER", &ord)){
            return 1;
        }

    }else if(fType == ".mp3"){
        char tagHeader[3];

        f.read(reinterpret_cast<char*>(&tagHeader), sizeof(char) * 3);

        if(tagHeader[0] != 0x49 && tagHeader[1] != 0x44 && tagHeader[2] != 0x33){
            cout << "\n Not ID3 : " << fileName << endl;
            f.close();
            return 1;
        }
        f.seekg(10);

        if(readMP3TagFrame(f, "TIT2", &title)){
            return 1;
        }
        f.seekg(10);
        
        if(readMP3TagFrame(f, "TRCK", &ord)){
            return 1;
        }
    }else{
        this->name = filesystem::path(fileName).extension().string() + " file format is not supported";

        this->order = 0;
        this->orderInAlbum = 0;
        f.close();
        return 0;
    }

    title = title.substr(0, title.find_last_not_of("\0"));
    if(title.rbegin()[0] == '\0'){
        title.pop_back();
    }
    this->name = title;

    ord = ord.substr(0, ord.find_last_not_of("\0"));
    ord = ord.substr(0, ord.find("/"));

    try {
        this->orderInAlbum = stoi(ord);
    } catch (int e) {
        this->orderInAlbum = 0;
    }
    f.close();
    return 0;
}
#endif

#if defined (PLATFORM_ANDROID) || defined (PLATFORM_WINDOWS)

int Track::readFile(string fileName){

    QFile f(QString::fromStdString(fileName));
    if(!f.open(QIODevice::ReadOnly)){
        return 1;
    }

    string title = "";
    string ord = "";
    string fType= "." + QFileInfo(QString::fromStdString(fileName)).suffix().toStdString();

    if(fType == ".flac"){
        char tagHeader[4];

        f.read(reinterpret_cast<char*>(&tagHeader), sizeof(char) * 4);

        if(tagHeader[0] != 0x66 && tagHeader[1] != 0x4c && tagHeader[2] != 0x61 && tagHeader[3] != 0x43){
            cout << "\n Not flac : " << fileName << endl;
            f.close();
            return 1;
        }
        f.seek(4);

        if(readFLACMetadataBlockQt(f, "TITLE", &title)){
            return 1;
        }
        f.seek(4);

        if(readFLACMetadataBlockQt(f, "TRACKNUMBER", &ord)){
            return 1;
        }

    }else if(fType == ".mp3"){
        char tagHeader[3];

        f.read(reinterpret_cast<char*>(&tagHeader), sizeof(char) * 3);

        if(tagHeader[0] != 0x49 && tagHeader[1] != 0x44 && tagHeader[2] != 0x33){
            cout << "\n Not ID3 : " << fileName << endl;
            f.close();
            return 1;
        }
        f.seek(10);

        if(readMP3TagFrameQt(f, "TIT2", &title)){
            return 1;
        }
        f.seek(10);
        
        if(readMP3TagFrameQt(f, "TRCK", &ord)){
            return 1;
        }
    }else{
        this->name = "." + QFileInfo(QString::fromStdString(fileName)).suffix().toStdString() + " file format is not supported";

        this->order = 0;
        this->orderInAlbum = 0;
        f.close();
        return 0;
    }

    title = title.substr(0, title.find_last_not_of("\0"));
    if(title.rbegin()[0] == '\0'){
        title.pop_back();
    }
    this->name = title;

    ord = ord.substr(0, ord.find_last_not_of("\0"));
    ord = ord.substr(0, ord.find("/"));

    try {
        this->orderInAlbum = stoi(ord);
    } catch (int e) {
        this->orderInAlbum = 0;
    }
    f.close();
    return 0;
}
#endif

void Track::printData(){
    cout << "Song " << this->name << ", track nr: "<< this->order << "\n";
}

