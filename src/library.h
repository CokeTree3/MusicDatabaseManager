#ifndef LIBRARY_H
#define LIBRARY_H
#include "sys_headers.h"

using namespace std;

class Track{
    public:
        int order;
        string name;
        
        Track(string name, int order);
        Track(int order, string filePath);

        void printData();
    private:
        int readFile(string fileName);
        int readMP3TagFrame(ifstream& f, const string& neededTagID, string* output);
        int readFLACMetadataBlock(ifstream& f, const string& neededBlockType, string* output);
};

class Album{
    public:
        string name;
        string coverPath;
        int trackCount;
        vector<Track> trackList;

        Album(string name);
        Album(string name, string dirPath);

        void printData();
        void printTracks();
        int addTrack(Track newTrack);
    private:
        int directoryToTracks(string path);
};


class Artist{
    public:
        int albumCount;
        string name;
        vector<Album> albumList;
        
        Artist(string name);
        Artist(string name, string dirPath);

        void printData();
        void displayData();
        int addAlbum(Album newAlbum);

    private:

        int directoryToAlbums(string path);

};



class Library{
    public:
        vector<Artist> artistList;

        Library(){
            //Artists = new vector<Artist>();
        }

        int addToLibrary(Artist newArtist);
        void printData();
        void displayData();

};


#endif