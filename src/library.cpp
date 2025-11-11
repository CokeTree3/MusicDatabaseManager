#include "library.h"


uint UcharArrayToUintLE(const unsigned char bytes[4]){
    return (uint)((bytes[0]) | ((bytes[1]) << 8) | ((bytes[2]) << 16) | ((bytes[3]) << 24));
}

void removeDir(string absolutePath){

#if defined (PLATFORM_LINUX) || defined (PLATFORM_WINDOWS)
    error_code ec;
    filesystem::remove_all(absolutePath, ec);
    if(ec){
        cout << "ERROR: couldn't remove directory or file: " << absolutePath << "\n Error: " << ec.message() << endl;
    }
#elif defined (PLATFORM_ANDROID)                                          // Dir removing on Android not implemented
    cout << "Directory removing is not supported" << endl;
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
    libPath = searchDir;
    int count = 1;

#if defined (PLATFORM_LINUX) || defined (PLATFORM_WINDOWS)
    for (auto i = filesystem::directory_iterator(searchDir); i != filesystem::directory_iterator(); ++i){
        if(!i->is_directory()){
            continue;
        }
        this->addToLibrary(i->path().lexically_relative(searchDir).string(), i->path().string());
        count++;
    }
#elif defined (PLATFORM_ANDROID)

    QDirIterator iter(QString::fromStdString(searchDir), QDir::NoDotAndDotDot | QDir::AllEntries);
    while(iter.hasNext()){
        QString path = iter.next();
        if(!iter.fileInfo().isDir()){
            continue;
        }
        this->addToLibrary(iter.fileName().toStdString(), iter.filePath().toStdString());
        count++;
    }
#endif

    jsonBuild();
    return 0;
}

void Library::jsonBuild(){
    if(serverActive){
        cout << "This library is currently used by an active server thread!/n Action not permitted!" << endl;
        return;
    }
    ofstream s("library.json", ios::binary);

    size_t count = artistList.size();
    libJson["LibraryPath"] = libPath;
    libJson["ArtistCount"] = count;
    libJson["Artists"] = json::array({});
    for(size_t i = 0; i < count; i++){
        libJson["Artists"].push_back(artistList[i]->getJsonStructure());
    }

    s << setw(4) << libJson;
    s.close();

    return;
}

int Library::jsonRead(){
    if(serverActive){
        cout << "This library is currently used by an active server thread!/n Action not permitted!" << endl;
        return 1;
    }
    string libFile = "library.json";

    if(access( libFile.c_str(), F_OK ) != -1){
        ifstream f(libFile, ios::binary);
        json data = json::parse(f);
        f.close();
        if(buildLibrary(data["LibraryPath"])){
            libSet = false;
            return 1;
        }
        libSet = true;
        return 0;

    }else{
        cout << "Creating a new Library JSON file\n";
        ofstream of(libFile, ios::binary);
        libJson["LibraryPath"] = libPath;
        of << setw(4) << libJson;
        of.close();
        return 1;
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
    for(size_t i = 0; i < jsonSource["Artists"].size(); i++){
        this->addToLibrary(jsonSource["Artists"][i]);
    }


    return 0;
}

int Library::find_diff(Library* remoteLib, json* jsonDiff){
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

int Library::syncWithServer(){
    if(remoteLibJson.empty()){
        cout << "Could not obtain server library file!\nRetry connection\n";
    }else{
        json diff;
        Library serverLib;
        serverLib.buildFromJson(remoteLibJson);
        serverLib.find_diff(this, &diff);
        
        if(diff.empty()){
            return 1;
        }

        ofstream f("libDiff.json", ios::binary);
        f << setw(4) << diff;
        f.close();

        cout << "starting fs operations\n";
        for(json artistJson : diff["Artists"]){
            string artistPath = this->libPath + "/" + string(artistJson["1Name"]);
            cout << artistPath << endl;
            if(artistJson.contains("Remove") && artistJson["Remove"] == true){
                cout << "removing artist " << artistPath << endl;
                removeDir(artistPath);
                continue;
            }
            bool artistFound = false;
            for (size_t i = 0; i < this->artistList.size(); i++){
                
                if(this->artistList[i]->name == string(artistJson["1Name"])){
                    artistFound = true;

                    for(json albumJson : artistJson["Albums"]){

                        string albumPath = artistPath + "/" + string(albumJson["1Name"]);
                        if(albumJson.contains("Remove") && albumJson["Remove"] == true){
                            cout << "removing album " << albumPath << endl;
                            removeDir(albumPath);
                            continue;
                        }

                        bool albumFound = false;
                        for(size_t j = 0; j < this->artistList[i]->albumList.size(); j++){

                            if(this->artistList[i]->albumList[j]->name == string(albumJson["1Name"])){
                                albumFound = true;

                                for(json trackJson : albumJson["Tracks"]){
                                    
                                    if(trackJson.contains("Remove") && trackJson["Remove"] == true){
                                        cout << "removing track " << string(trackJson["FileName"]) << endl;
                                        removeFile(albumPath + "/" + string(trackJson["FileName"]));
                                        continue;
                                    }

                                    bool trackFound = false;
                                    for(size_t k = 0; k < this->artistList[i]->albumList[j]->trackList.size(); k++){                // Mignt not be needed as the Diff will only contain tracks that need downloading or removing
                                        
                                        if(this->artistList[i]->albumList[j]->trackList[k]->name == string(trackJson["1Name"])){    // Matches will never be found
                                            cout << "bad!!" << endl;
                                            trackFound = true;
                                        }
                                        
                                    }
                                    if(!trackFound){
                                        createNewFile(albumPath + "/", string(trackJson["FileName"]));
                                    }
                                }
                            }
                        }
                        if(!albumFound){
                            makeDir(albumPath);

                            for(json trackJson : albumJson["Tracks"]){
                                createNewFile(albumPath + "/", string(trackJson["FileName"]));
                            }
                        }
                    }
                }
            }
            if(!artistFound){
                makeDir(artistPath);
                for(json albumJson : artistJson["Albums"]){
                    makeDir(artistPath + "/" + string(albumJson["1Name"]));

                    for(json trackJson : albumJson["Tracks"]){
                        createNewFile(artistPath + "/" + string(albumJson["1Name"]) + "/", string(trackJson["FileName"]));
                    }
                }
            }
        }
        
    }


    return 0;
}

#if defined (PLATFORM_LINUX) || defined (PLATFORM_WINDOWS)
int Library::createNewFile(string path, string fName){

#if defined (PLATFORM_WINDOWS)
    filesystem::path fPath = filesystem::u8path(path + fName);
    ofstream trackFile(fPath, ios::binary);
#endif

#if defined (PLATFORM_LINUX)
    ofstream trackFile(path + fName, ios::binary);
#endif

    if(!trackFile.is_open()){
        cout << "error creating a new file " << path + fName << endl;
        return 1;
    }
    string relativePath = string(path+fName).substr(libPath.length());

    vector<char> buf;

    requestTrack(relativePath, buf);

    trackFile.write(buf.data(), buf.size());

    trackFile.close();
    return 0;
}
#endif

#if defined (PLATFORM_ANDROID)
int Library::createNewFile(string path, string fName){
    string type = fName.substr(fName.rfind('.') + 1, fName.npos);
    string relativePath = path.substr(libPath.length() + 1);

    if(!permsObtained){
        requestPermissions();
    }
    vector<char> buf;
    string relativePathWName = string(path+fName).substr(libPath.length());
    requestTrack(relativePathWName, buf);

    int res = createFileAndroid(fName, relativePath, buf.data(), buf.size());

    if(res != 0){
        cout << "Error creating track file: " << path + fName <<  endl;                 // error msg notif on android
    }
    return 0;
}
#endif

int Library::addToLibrary(string name, string dirPath){
    artistList.push_back(make_unique<Artist>(name, dirPath));
    return artistList.size();
}

int Library::addToLibrary(json jsonSource){

    artistList.push_back(make_unique<Artist>(jsonSource));
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
    this->name = name;
    albumCount = 0;
}

Artist::Artist(string name, string dirPath){
    this->name = name;
    if(directoryToAlbums(dirPath)){
        //throw error
    }
    albumCount = albumList.size();
}

Artist::Artist(json jsonSource){
    this->name = jsonSource["1Name"];
    
    for(size_t i = 0; i < jsonSource["Albums"].size(); i++){
        this->addAlbum(jsonSource["Albums"][i]);
    }

    this->albumCount = this->albumList.size();
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
    return albumCount + 1;
}

int Artist::addAlbum(json jsonSource){
    albumList.push_back(make_unique<Album>(jsonSource));
    return albumCount + 1;
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
    this->name = name;

#if defined (PLATFORM_LINUX) || defined (PLATFORM_WINDOWS)
    this->coverPath = filesystem::current_path().string() + "/assets/missingCov.jpg";

#elif defined (PLATFORM_ANDROID)                      // TODO add missing cover placeholder icon
    this->coverPath = "";
#endif
    trackCount = 0;
}

Album::Album(string name, string dirPath){
    this->name = name;

    if(directoryToTracks(dirPath)){
        //throw error
    }
    trackCount = trackList.size();

#if defined (PLATFORM_LINUX) || defined (PLATFORM_WINDOWS)

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
    }

#endif
}

Album::Album(json jsonSource){
    this->name = jsonSource["1Name"];
    for(size_t i = 0; i < jsonSource["Tracks"].size(); i++){
        addTrack(jsonSource["Tracks"][i]);
    }
    this->coverPath = "";
    this->trackCount = this->trackList.size();
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

                (*jsonDiff)["Tracks"].push_back(trackToRemove);

            }
        }
        return true;
    }
    return false;
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
    return trackList.size();
}

int Album::addTrack(json jsonSource){
    trackList.push_back(make_unique<Track>(jsonSource));
    trackCount++;
    return trackList.size();
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
                  return a->orderInAlbum < b->orderInAlbum;
              });
    return 0;
}



/*
*   Track Class Functions
*/
Track::Track(string name, int order){
    this->name = name;
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
    this->name = jsonSource["1Name"];
    this->orderInAlbum = jsonSource["Order"];
    this->order = this->orderInAlbum;
    this->fileName = jsonSource["FileName"];
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
        }else{
            data["1Name"] = this->name;
        }
    }catch(...){
        cout << this->name << "BADBAD" << endl;
    }
    
    data["Order"] = this->orderInAlbum;
    data["FileName"] = this->fileName;

    return data;
}

#if defined (PLATFORM_LINUX)// || defined (PLATFORM_WINDOWS)                                Windows currently temporarly moved to QT approach
int Track::readMP3TagFrame(ifstream& f, const string& neededTagID, string* output){
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
        readMP3TagFrame(f, neededTagID, output);
        return 0;
    }
    
    
    f.ignore(3);
    output->resize(frameSize);

    f.read(&(*output)[0], frameSize);

    return 0;
}

int Track::readFLACMetadataBlock(ifstream& f, const string& neededBlockType, string* output){

    unsigned char header[4];
    f.read(reinterpret_cast<char*>(&header), sizeof(char) * 4);
    uint blockSize = (header[1] << 16) | (header[2] << 8) | header[3];
    
    if((header[0] & 0x7F) != 4){
        if(header[0] & 0x80){
            cout << "FLAC file does not contain information: " + neededBlockType << endl;
            return 1;
        }
        f.ignore(blockSize);
        
        readFLACMetadataBlock(f, neededBlockType, output);
        return 0;
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
        readMP3TagFrameQt(f, neededTagID, output);
        return 0;
    }
    
    
    f.skip(3);
    output->resize(frameSize);

    f.read(&(*output)[0], frameSize);

    return 0;
}

int Track::readFLACMetadataBlockQt(QFile& f, const string& neededBlockType, string* output){

    unsigned char header[4];
    f.read(reinterpret_cast<char*>(&header), sizeof(char) * 4);

    uint blockSize = (header[1] << 16) | (header[2] << 8) | header[3];
    
    if((header[0] & 0x7F) != 4){
        if(header[0] & 0x80){
            cout << "FLAC file does not contain title information" << endl;
            return 1;
        }
        f.skip(blockSize);
        
        readFLACMetadataBlockQt(f, neededBlockType, output);
        return 0;
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
            //throw error
        }
        f.seekg(4);

        if(readFLACMetadataBlock(f, "TRACKNUMBER", &ord)){
            //throw error
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
            //throw error
        }
        f.seekg(10);
        
        if(readMP3TagFrame(f, "TRCK", &ord)){
            cout << "Error\n";
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
            //throw error
        }
        f.seek(4);

        if(readFLACMetadataBlockQt(f, "TRACKNUMBER", &ord)){
            //throw error
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
            //throw error
        }
        f.seek(10);
        
        if(readMP3TagFrameQt(f, "TRCK", &ord)){
            cout << "Error\n";
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

