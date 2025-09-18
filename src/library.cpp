#include "library.h"

/*
*   Library Class Functions
*/
int Library::addToLibrary(Artist newArtist){
    artistList.push_back(newArtist);
    return artistList.size();
}

void Library::printData(){
    cout << "Artists in the database: \n";
    int count = 1;
    for(Artist entry : artistList){
        cout << count << ".  " << entry.name << endl;
        count++;
    }
}

void Library::displayData(){
    this->printData();

    cout << "Select which artist to display: ";
    int nr = 0;
    while (!(cin >> nr) || nr < 1 || nr > (int)artistList.size()) {
        cout << "Invalid input. Please enter a valid integer: ";
        cin.clear(); 
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }

    artistList[nr-1].displayData();
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


void Artist::printData(){
    cout << name << " albums - \n";
    for(int i = 0; i < albumCount; i++){
        cout << i + 1 << ". " << albumList[i].name << endl;
    }
}

void Artist::displayData(){
    this->printData();

    cout << "Select which album to display: ";
    int nr = 0;
    while (!(cin >> nr) || nr < 1 || nr > (int)albumList.size()) {
        cout << "Invalid input. Please enter a valid integer: ";
        cin.clear(); 
        cin.ignore(numeric_limits<streamsize>::max(), '\n');
    }
    albumList[nr-1].printData();
}

int Artist::addAlbum(Album newAlbum){
    albumList.push_back(newAlbum);
    return albumList.size();
}

int Artist::directoryToAlbums(string path){
    if(path == ""){
        return 1;
    }

    for (auto i = filesystem::directory_iterator(path); i != filesystem::directory_iterator(); ++i){
        if(!i->is_directory()){
            continue;
        }
        
        Album newAlbum(i->path().lexically_relative(path).string(), i->path().string());
        addAlbum(newAlbum);
    }
    return 0;
}



/*
*   Album Class Functions
*/
Album::Album(string name){
    this->name = name;
    coverPath = "";
    trackCount = 0;
}

Album::Album(string name, string dirPath){
    this->name = name;
    this->coverPath = "";
    if(directoryToTracks(dirPath)){
        //throw error
    }
    trackCount = trackList.size();
}


void Album::printData(){
    cout << name << " with "<< trackCount << " Tracks: \n";
    this->printTracks();
}

void Album::printTracks(){
    for(int i = 0; (size_t)i < trackList.size(); i++){
        cout << i+1 << ". " << trackList[i].name << endl;
    }
}

int Album::addTrack(Track newTrack){
    trackList.push_back(newTrack);
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

        Track newTrack(trackOrder, i->path().string());

        addTrack(newTrack);
        trackOrder++;
    }
    return 0;
}



/*
*   Track Class Functions
*/
Track::Track(string name, int order){
    this->name = name;
    this->order = order;
}

Track::Track(int order, string filePath){
    this->order = order;
    readFile(filePath);
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
    frameSize--;
    
    f.ignore(3);
    output->resize(frameSize);

    f.read(&(*output)[0], frameSize);

    return 0;
}

int Track::readFile(string fileName){
    if(filesystem::path(fileName).extension().string() == ".flac"){

        // .flac audio files are skipped for now
        this->name = "FLAC files are not yet supported";
        return 0;
    }

    ifstream f(fileName, ios::binary);
    if(!f.is_open()){
        return 1;
    }

    char tagHeader[3];

    f.read(reinterpret_cast<char*>(&tagHeader), sizeof(char) * 3);

    if(tagHeader[0] != 0x49 && tagHeader[1] != 0x44 && tagHeader[2] != 0x33){
        cout << "\n not ID3 : " << fileName << endl;
        f.close();
        return 1;
    }
    f.seekg(10);

    string title = "";
    if(readMP3TagFrame(f, "TIT2", &title)){
        //throw error
    }
    
    this->name = title;

    f.close();
    return 0;
}

void Track::printData(){
    cout << "Song " << this->name << ", track nr: "<< this->order << "\n";
}

