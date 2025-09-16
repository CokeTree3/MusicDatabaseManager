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



/*
*   Artist Class Functions
*/
Artist::Artist(string name){
    this->name = name;
    albumCount = 0;
}

Artist::Artist(string name, string dirPath){
    this->name = name;
    directoryToAlbums(dirPath);
    albumCount = albumList.size();
}


void Artist::printData(){
    cout << "Artist " << name << " with "<< albumCount << " Albums\n";
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
        
        //cout << "\t" << i->path().lexically_relative(path).string() << endl;
        Album newAlbum(i->path().lexically_relative(path).string(), i->path().string());

        // build album

        newAlbum.printData();
        newAlbum.printTracks();
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
    
    directoryToTracks(dirPath);
    trackCount = trackList.size();
}


void Album::printData(){
    cout << "Album " << name << " with "<< trackCount << " Tracks\n";
}

void Album::printTracks(){
    cout << "Tracks in " << name << ": \n";
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

        //cout << "\t\t" << i->path().lexically_relative(path).string() << endl;
        Track newTrack(i->path().lexically_relative(path).string(), trackOrder);

        // build track

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

void Track::printData(){
    cout << "Song " << this->name << ", track nr: "<< this->order << "\n";
}