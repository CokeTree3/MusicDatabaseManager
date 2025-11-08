#ifndef LIBRARY_H
#define LIBRARY_H
#include "sys_headers.h"
#include "networking.h"


class Track{
    public:
        int order;
        int orderInAlbum;
        string name;
        string fileName;
        
        Track(string name, int order);
        Track(int order, string filePath);
        Track(json jsonSource);
        bool equals(unique_ptr<Track>& rhs);

        void printData();
        json getJsonStructure();
    private:
        int readFile(string fileName);
        

        #if defined (PLATFORM_LINUX)// || defined (PLATFORM_WINDOWS)                        Windows currently temporarly moved to QT approach                
            int readMP3TagFrame(ifstream& f, const string& neededTagID, string* output);
            int readFLACMetadataBlock(ifstream& f, const string& neededBlockType, string* output);
        #endif

        #if defined (PLATFORM_ANDROID) || defined (PLATFORM_WINDOWS)
            int readMP3TagFrameQt(QFile& f, const string& neededTagID, string* output);
            int readFLACMetadataBlockQt(QFile& f, const string& neededBlockType, string* output);
        #endif
};

class Album{
    public:
        string name;
        string coverPath;
        size_t trackCount;
        vector<unique_ptr<Track>> trackList;

        Album(string name);
        Album(string name, string dirPath);
        Album(json jsonSource);
        bool equals(unique_ptr<Album>& rhs, json* jsonDiff);

        void printData();
        void printTracks();
        int addTrack(int order, string filePath);
        int addTrack(json jsonSource);
        json getJsonStructure();
        json getEmptyJsonStructure();

    private:
        int directoryToTracks(string path);
};


class Artist{
    public:
        size_t albumCount = 0;
        string name;
        vector<unique_ptr<Album>> albumList;
        
        Artist(string name);
        Artist(string name, string dirPath);
        Artist(json jsonSource);

        bool equals(unique_ptr<Artist>& rhs, json* jsonDiff);

        void printData();
        void displayData();
        int addAlbum(string name, string dirPath);
        int addAlbum(json jsonSource);
        json getJsonStructure();
        json getEmptyJsonStructure();
        

    private:

        int directoryToAlbums(string path);

};



class Library{
    public:
        vector<unique_ptr<Artist>> artistList;
        json libJson;
        string libPath;
        json remoteLibJson;
        bool permsObtained;

        Library(){
            permsObtained = false;
        }
        void jsonBuild();
        int addToLibrary(string name, string dirPath);
        int addToLibrary(json jsonSource);
        void resetLibrary();
        int buildLibrary(string searchDir);
        int buildFromJson(json jsonSource);

        int jsonRead();

        int find_diff(Library* remoteLib, json* jsonDiff);
        int syncWithServer();

        void printData();
        void displayData();
    private:
        int createNewFile(string path, string fName);

};


#endif