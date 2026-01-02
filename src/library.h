/**
    Defines the Library classes which manage the music tracks and related info within the application
*/

#ifndef LIBRARY_H
#define LIBRARY_H
#include "sys_headers.h"
#include "networking.h"

// Predefined to allow backreferencing parent class
class Library;
class Artist;
class Album;

class Track{
    public:
        int orderInAlbum;
        string name;
        string fileName;
        bool toBeRemoved = false;
        

        /**
        *   @brief Track constructor with a link to a track file
        *   @param order Track order in the parent Album
        *   @param filePath Absolute path to a audio track file(MP3 or FLAC)
        */
        Track(int order, string filePath);

        /**
        *   @brief Track constructor from a json object
        *   @param jsonSource json object that contains track data constructed by a Track.getJsonStructure() function call
        */
        Track(json jsonSource);

        /**
        *   @brief Track constructor that duplicates another Track object
        *   @param toCopyFrom A valid reference to another Track object 
        */
        Track(Track& toCopyFrom);

        
        /**
        *   @brief Compares this track to another
        *   @param rhs A reference to a Track object pointer to compare this object to
        *   @return True if all parameteres of this object match the Track provided as the argument, False otherwise
        */
        bool equals(unique_ptr<Track>& rhs);

        /**
        *   @brief Implements the differences from the sync process
        *       Creates and downloads the audio file, or deletes it as needed
        *   @note Needs to be called on the Diff Library tracks not the local Library.
        *
        *   @param mainLibAlbum A reference to the Local library(That have the track file links) parent album of this Track
        *   @param rootPath Absolute path to the track file
        *   @param libPath Absolute path to the local library directory
        *   @return 0 on success, >0 on error 
        */
        int implementDiff(Album& mainLibAlbum, string rootPath, const string libPath);

        /**
        *   @brief Prints the data of this Track
        */
        void printData();

        /**
        *   @brief Creates a json object containing the track data
        *   @return Returns a JSON object
        */
        json getJsonStructure();

        /**
        *   @brief Fetches the embeded cover image from the audio file
        *   @param buf A reference to a buffer to write the image data into
        *   @param rootPath Absolute path to the track file to read from
        *   @return 0 on success, >0 on error(Including missing embed)
        */
        int getEmbededImage(vector<unsigned char>& buf, string rootPath);

    private:

        /**
        *   @brief Reads a audio file and fetches needed metadata from it
        *   @param fileName Absolute path to the track file to read from
        *   @return 0 on success, >0 on error
        */
        int readFile(string fileName);

#if defined (PLATFORM_LINUX) || defined (PLATFORM_WINDOWS)     
    
        /**
        *   @brief Finds and reads the requested metadata tag from an open MP3 file
        *   @param f An open filestream to a MP3 file containing an ID3v2 metadata header
        *   @param neededTagID Tag ID to search for
        *   @param ID3Version Either 3 or 4, indicating ID3V2.3 or 2.4 header version
        *   @param output A pointer to a valid string to write the metadata field data into
        *   @return 0 on success, >0 on error
        */
        int readMP3TagFrame(ifstream& f, const string& neededTagID, char ID3Version, string* output);

        /**
        *   @brief Finds and reads the requested metadata block from an open FLAC file
        *   @param f An open filestream to a FLAC file
        *   @param neededInfoType The needed VORBIS_COMMENT data field, "" if requesting different block 
        *   @param neededBlock 4 for request for VORBIS_COMMENT data, or 6 for the embeded image data
        *   @param output A pointer to a valid string to write the metadata field data into
        *   @return 0 on success, >0 on error
        */
        int readFLACMetadataBlock(ifstream& f, const string& neededInfoType, uint8_t neededBlock, string* output);
#endif

#if defined (PLATFORM_ANDROID)  // Android compatibility to be moved to dedicated project, functions serve the same purpouse as the above alternatives
        int readMP3TagFrameQt(QFile& f, const string& neededTagID, char ID3Version, string* output);
        int readFLACMetadataBlockQt(QFile& f, const string& neededInfoType, uint8_t neededBlock, string* output);
#endif
};

class Album{
    public:
        string name;
        size_t trackCount = 0;
        vector<unique_ptr<Track>> trackList;
        vector<unsigned char> coverBuf;
        string coverPath;
        bool toBeRemoved = false;


        /**
        *   @brief Album constructor with a link to a directory containing Track files
        *   @param filePath Absolute path to an album directory
        */
        Album(string dirPath);

        /**
        *   @brief Album constructor from a json object
        *   @param jsonSource json object that contains album data constructed by an Album.getJsonStructure() function call
        */
        Album(json jsonSource);

        /**
        *   @brief Album constructor that duplicates another Album object
        *   @param toCopyFrom A valid reference to another Album class object 
        *   @param fullPath Absolute path to the Album Directory
        *   @param libPath Absolute path to the local library root
        */
        Album(Album& toCopyFrom, string fullPath = "", string libPath = "");


        /**
        *   @brief Compares this album to another
        *   @param rhs A reference to an Album object pointer to compare this object to
        *   @param jsonDiff Pointer to an empty json object to fill if @param rhs is not equal to this objec
        *   @return True if all parametres of this object match the Album provided as the argument, False otherwise
        */
        bool equals(unique_ptr<Album>& rhs, json* jsonDiff);



        /**
        *   @brief Prints the data of this Album
        */
        void printData();

        /**
        *   @brief Prints the Track list of this Album
        */
        void printTracks();



        /**
        *   @brief Constructs and adds a track to the track list of this Album
        *       Takes the arguments of the matching Track constructor
        *   @param order Track order in the parent Album
        *   @param filePath Absolute path to a audio track file(MP3 or FLAC)
        *   @return 0 on success, >0 on error
        */
        int addTrack(int order, string filePath);

        /**
        *   @brief Constructs and adds a track to the track list of this Album
        *       Takes the arguments of the matching Track constructor
        *   @param jsonSource json object that contains track data constructed by a Track.getJsonStructure() function call
        *   @return 0 on success, >0 on error
        */
        int addTrack(json jsonSource);

        /**
        *   @brief Constructs and adds a track to the track list of this Album
        *       Takes the arguments of the matching Track constructor
        *   @param toCopyFrom A valid reference to another Track object 
        *   @return 0 on success, >0 on error
        */
        int addTrack(Track& toCopyFrom);

        /**
        *   @brief Removes a track from the track list if found
        *   @param name Name of the track to remove
        *   @param indexInList index of the track to remove
        *   @note Removes based on the index of the track, however the names must match 
        *   @return 0 on success, >0 on error
        */
        int removeTrack(string name, int indexInList);

        /**
        *   @brief Sorts the tracks in the track list based on Track.orderInAlbum
        */
        void sortTracks();

        /**
        *   @brief Tries to obtain the cover image of the album
        *   @param rootPath Absolute path to the album directory
        */
        void setCover(string rootPath);

        /**
        *   @brief Implements the differences from the sync process
        *   @note Needs to be called on the Diff Library albums not the local Library.
        *
        *   @param mainLibArtist A reference to the Local library(That have the track file links) parent artist of this Album
        *   @param rootPath Absolute path to the album directory
        *   @param libPath Absolute path to the local library directory
        *   @return 0 on success, >0 on error 
        */
        int implementDiff(Artist& mainLibArtist, string rootPath, const string libPath);

        /**
        *   @brief Creates a json object containing the album and track list data
        *   @return Returns a JSON object
        */
        json getJsonStructure();

        /**
        *   @brief Creates a basic json object with empty data fields
        *   @return Returns a JSON object
        */
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
        

        /**
        *   @brief Artist constructor with a link to a directory containing Album subdirectories
        *   @param filePath Absolute path to an artist directory
        */
        Artist(string dirPath);

        /**
        *   @brief Artist constructor from a json object
        *   @param jsonSource json object that contains artist data constructed by an Artist.getJsonStructure() function call
        */
        Artist(json jsonSource);

        /**
        *   @brief Artist constructor that duplicates another Artist object
        *   @param toCopyFrom A valid reference to another Artist class object 
        *   @param fullPath Absolute path to the Artist Directory
        *   @param libPath Absolute path to the local library root
        */
        Artist(Artist& toCopyFrom, string fullPath = "", string libPath = "");

        
        
        /**
        *   @brief Compares this artist to another
        *   @param rhs A reference to an Artist object pointer to compare this object to
        *   @param jsonDiff Pointer to an empty json object to fill if @param rhs is not equal to this objec
        *   @return True if all parametres of this object match the Artist provided as the argument, False otherwise
        */
        bool equals(unique_ptr<Artist>& rhs, json* jsonDiff);

        /**
        *   @brief Prints the data of this Artist
        */
        void printData();

        /**
        *   @brief Prints the Album list of this Artist
        */
        void displayData();



        /**
        *   @brief Constructs and adds an album to the album list of this Artist
        *       Takes the arguments of the matching Album constructor
        *   @param filePath Absolute path to an album directory
        *   @return 0 on success, >0 on error
        */
        int addAlbum(string dirPath);

        /**
        *   @brief Constructs and adds an album to the album list of this Artist
        *       Takes the arguments of the matching Album constructor
        *   @param jsonSource json object that contains album data constructed by an Album.getJsonStructure() function call
        *   @return 0 on success, >0 on error
        */
        int addAlbum(json jsonSource);

        /**
        *   @brief Constructs and adds an album to the album list of this Artist
        *       Takes the arguments of the matching Album constructor
        *   @param toCopyFrom A valid reference to another Album class object 
        *   @param fullPath Absolute path to the Album Directory
        *   @param libPath Absolute path to the local library root
        *   @return 0 on success, >0 on error
        */
        int addAlbum(Album& toCopyFrom, string fullPath = "", string libPath = "");

        /**
        *   @brief Removes an album from the album list if found
        *   @param name Name of the album to remove
        *   @param indexInList index of the album to remove
        *   @note Removes based on the index of the album, however the names must match 
        *   @return 0 on success, >0 on error
        */
        int removeAlbum(string name, int indexInList);



        /**
        *   @brief Implements the differences from the sync process
        *   @note Needs to be called on the Diff Library artists not the local Library.
        *
        *   @param mainLibArtist A reference to the Local library(That have the track file links)
        *   @param rootPath Absolute path to the artist directory
        *   @param libPath Absolute path to the local library directory
        *   @return 0 on success, >0 on error 
        */
        int implementDiff(Library& mainLib, string rootPath, const string libPath);

        /**
        *   @brief Creates a json object containing the artist and track list data
        *   @return Returns a JSON object
        */
        json getJsonStructure();

        /**
        *   @brief Creates a basic json object with empty data fields
        *   @return Returns a JSON object
        */
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

        
        /**
        *   @brief Constructs and adds an artist to the artist list of this Library
        *       Takes the arguments of the matching Artist constructor
        *   @param dirPath Absolute path to the artist directory
        *   @return 0 on success, >0 on error
        */
        int addToLibrary(string dirPath);

        /**
        *   @brief Constructs and adds an artist to the artist list of this Library
        *       Takes the arguments of the matching Artist constructor
        *   @param jsonSource json object that contains artist data constructed by an Artist.getJsonStructure() function call
        *   @return 0 on success, >0 on error
        */
        int addToLibrary(json jsonSource);

        /**
        *   @brief Constructs and adds an artist to the artist list of this Library
        *       Takes the arguments of the matching Artist constructor
        *   @param toCopyFrom A valid reference to another Artist class object 
        *   @param fullPath Absolute path to the Artist Directory
        *   @param libPath Absolute path to the local library root
        *   @return 0 on success, >0 on error
        */
        int addToLibrary(Artist& toCopyFrom, string fullPath = "", string libPath = "");

        /**
        *   @brief Removes an artist from the artist list if found
        *   @param name Name of the artist to remove
        *   @param indexInList index of the artist to remove
        *   @note Removes based on the index of the artist, however the names must match 
        *   @return 0 on success, >0 on error
        */
        int removeFromLibrary(string name, int indexInList);


        /**
        *   @brief Clears the artist list of all entries 
        */
        void resetLibrary();




        /**
        *   @brief Creates and writes into a .json file the structure of the this library
        *   @return 0 on success, >0 on error  
        */
        int jsonBuild();

        /**
        *   @brief Builds the Library structure(Artists -> Albums -> Tracks) from a local directory path
        *   @param searchDir Absolute path to the library directory (containing Artist subdirectories)
        *   @return 0 on success, >0 on error  
        */
        int buildLibrary(string searchDir);

        /**
        *   @brief Builds the Library structure(Artists -> Albums -> Tracks) from a valid json library 
        *   @param searchDir Valid json structure of the library (constructed by Library.jsonBuild())
        *   @return 0 on success, >0 on error  
        */
        int buildFromJson(json jsonSource);

        /**
        *   @brief Reads a library.json file and constructs this library from the LibraryPath directory value in it
        *   @note The library.json file should exist in the working directory
        *   @return 0 on success, >0 on error  
        */
        int jsonRead();




        /**
        *   @brief Creates a diff between this library and a second one
        *   @param remoteLib A pointer to a different Library to compare to
        *   @param jsonDiff A valid pointer to a json to write the diff into
        *   @return 0 on success, >0 on error  
        */
        int findDiff(Library* remoteLib, json* jsonDiff);

        /**
        *   @brief Generates the diff between this Library and a second one (Library.remoteLibJson)
        *   @note Library.remoteLibJson for the current Library need to be set before calling
        *   @param diffLib A reference to an empty Library to implement the diff into
        *   @return 0 on success, >0 on error  
        */
        int generateDiff(Library &diffLib);

        /**
        *   @brief Implements the diff into this (local) Library
        *   @param diffLib A reference to the diff library
        *   @return 0 on success, >0 on error  
        */
        int implementDiff(Library &diffLib);



        
        /**
        *   @brief Prints the data of this Library
        */
        void printData();

        /**
        *   @brief Prints the Artist list of this Library with interactivity
        */
        void displayData();

};


#endif