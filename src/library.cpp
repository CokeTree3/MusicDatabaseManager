#include "library.h"
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <vector>


uint UcharArrayToUintLE(const unsigned char bytes[4]){
    return (uint)((bytes[0]) | ((bytes[1]) << 8) | ((bytes[2]) << 16) | ((bytes[3]) << 24));
}



/*
*   Library Class Functions
*/
int Library::buildLibrary(string searchDir){
    if(searchDir == ""){
        return 1;
    }
    libPath = searchDir;
    int count = 1;
    for (auto i = filesystem::directory_iterator(searchDir); i != filesystem::directory_iterator(); ++i){
        if(!i->is_directory()){
            continue;
        }

        this->addToLibrary(i->path().lexically_relative(searchDir).string(), i->path().string());

        count++;
    }

    //jsonBuild();
    return 0;
}

void Library::jsonBuild(){
    ofstream s("library.json");

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
    string libFile = "library.json";

    if(access( libFile.c_str(), F_OK ) != -1){
        ifstream f(libFile);
        json data = json::parse(f);
        f.close();
        buildLibrary(data["LibraryPath"]);

        return 0;

    }else{
        cout << "Creating a new Library JSON file\n";
        ofstream of(libFile);
        libJson["LibraryPath"] = libPath;
        of << setw(4) << libJson;
        of.close();
        return 1;
    }
    
}

int Library::buildFromJson(json jsonSource){
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
    jsonBuild();
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

    for (auto i = filesystem::directory_iterator(path); i != filesystem::directory_iterator(); ++i){
        if(!i->is_directory()){
            continue;
        }
        
        addAlbum(i->path().lexically_relative(path).string(), i->path().string());
    }
    return 0;
}



/*
*   Album Class Functions
*/
Album::Album(string name){
    this->name = name;
    this->coverPath = filesystem::current_path().string() + "/assets/missingCov.jpg";
    trackCount = 0;
}

Album::Album(string name, string dirPath){
    this->name = name;
    this->coverPath = filesystem::current_path().string() + "/assets/missingCov.jpg";
    if(directoryToTracks(dirPath)){
        //throw error
    }
    trackCount = trackList.size();

    if(filesystem::exists(filesystem::path(dirPath + "/cover.jpg"))){
        coverPath = dirPath + "/cover.jpg";
    }
    else if(filesystem::exists(filesystem::path(dirPath + "/cover.png"))){
        coverPath = dirPath + "/cover.png";
    }

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
}

Track::Track(int order, string filePath){
    this->order = order;
    if(readFile(filePath)){
        
    }
    this->path = filePath;
}

Track::Track(json jsonSource){
    this->name = jsonSource["1Name"];
    this->orderInAlbum = jsonSource["Order"];
    this->order = this->orderInAlbum;
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
        //data["1Name"] = this->name;
    }catch(...){
        cout << this->name << "BADBAD" << endl;
    }
    
    data["Order"] = this->orderInAlbum;

    return data;
}

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

    /*for(int i = 0; i < 4; i++){
        printf("%x ", header[i]);
    }*/
    uint blockSize = (header[1] << 16) | (header[2] << 8) | header[3];
    //printf("\n %u",blockSize);
    
    if((header[0] & 0x7F) != 4){
        if(header[0] & 0x80){
            cout << "FLAC file does not contain title information" << endl;
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

int Track::readFile(string fileName){

    ifstream f(fileName, ios::binary);
    if(!f.is_open()){
        return 1;
    }

    string title = "";
    string ord = "";

    if(filesystem::path(fileName).extension().string() == ".flac"){
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

    }else if(filesystem::path(fileName).extension().string() == ".mp3"){
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

void Track::printData(){
    cout << "Song " << this->name << ", track nr: "<< this->order << "\n";
}

