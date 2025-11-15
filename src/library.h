#ifndef LIBRARY_H
#define LIBRARY_H
#include "sys_headers.h"
#include "networking.h"

class Library;
class Artist;
class Album;

class Track{
    public:
        int order;
        int orderInAlbum;
        string name;
        string fileName;
        bool toBeRemoved = false;
        
        Track(string name, int order);
        Track(int order, string filePath);
        Track(json jsonSource);
        Track(Track& toCopyFrom);

        bool equals(unique_ptr<Track>& rhs);
        int implementDiff(Album& mainLibAlbum, string rootPath, const string libPath);
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
        bool toBeRemoved = false;
        vector<unique_ptr<Track>> trackList;

        Album(string name);
        Album(string name, string dirPath);
        Album(json jsonSource);
        Album(Album& toCopyFrom, string fullPath = "", string libPath = "");

        bool equals(unique_ptr<Album>& rhs, json* jsonDiff);

        void printData();
        void printTracks();
        int addTrack(int order, string filePath);
        int addTrack(json jsonSource);
        int addTrack(Track& toCopyFrom);
        int removeTrack(string name, int indexInList);

        int implementDiff(Artist& mainLibArtist, string rootPath, const string libPath);
        json getJsonStructure();
        json getEmptyJsonStructure();
        

    private:
        int directoryToTracks(string path);
};


class Artist{
    public:
        size_t albumCount = 0;
        string name;
        bool toBeRemoved = false;
        vector<unique_ptr<Album>> albumList;
        
        Artist(string name);
        Artist(string name, string dirPath);
        Artist(json jsonSource);
        Artist(Artist& toCopyFrom, string fullPath = "", string libPath = "");

        bool equals(unique_ptr<Artist>& rhs, json* jsonDiff);

        void printData();
        void displayData();

        int addAlbum(string name, string dirPath);
        int addAlbum(json jsonSource);
        int addAlbum(Album& toCopyFrom, string fullPath = "", string libPath = "");
        int removeAlbum(string name, int indexInList);

        int implementDiff(Library& mainLib, string rootPath, const string libPath);

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
        bool serverActive;

        Library(){
            permsObtained = false;
            serverActive = false;
        }
        void jsonBuild();

        int addToLibrary(string name, string dirPath);
        int addToLibrary(json jsonSource);
        int addToLibrary(Artist& toCopyFrom, string fullPath = "", string libPath = "");
        int removeFromLibrary(string name, int indexInList);

        void resetLibrary();
        
        int buildLibrary(string searchDir);
        int buildFromJson(json jsonSource);

        int jsonRead();

        int find_diff(Library* remoteLib, json* jsonDiff);
        int syncWithServer();

        void printData();
        void displayData();

};


#endif