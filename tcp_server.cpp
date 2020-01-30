#include <iostream>
#include <string>
#include <unistd.h>
#include <fstream>
#include <chrono>
#include <vector>
#include <map>
#include <algorithm>

// custom scripts
#include "server_util.h"

using namespace std;

// struct to act as cache
// contents maps a file to its contents
// sizes maps a file to its size
// times maps a file to the last access time of that file
// total size is the total size of the cache
// NOTE: The cache is guarenteed to hold 60 MB of file contents, therefore
// I only track the map content's char* size rather than the additional
// overhead of the sizes, times, and total size data structures
struct Cache{
    // actual content of the file
    map<string, char*> *contents;

    // size in bytes of each file
    map<string, int> *sizes;

    // last access time of each file
    map<string, double> *times;

    // total size of all fizes stored in cache
    int total_size;
};

char * retrieve_file(const string &file_name, const string &directory, Cache &cache);
bool update_cache(const string &file_name, ifstream &client_file, Cache &cache);
int get_file_size(ifstream &client_file);
Cache* init_cache();
bool check_cache(const string &file_name, Cache &cache);
bool send_from_disk(const int &client_socket, ifstream &client_file, string &absolute_path);
int make_room_LRU(Cache &cache, int client_file_size);
void remove_cache_file(string &file_name, Cache &cache);

// Buffer size for reading file
const int BUFFSIZE = 100;

// set max size to 64 MB
const int MAX_CACHE_SIZE = 2000;//64*1048576;

// start time of the program (used for LRU cache implementation)
chrono::_V2::system_clock::time_point START_TIME;

int main(int argc, char ** argv){
    // get the start execution time
	START_TIME = chrono::high_resolution_clock::now();

    if(argc != 3){
        printf("ERROR: Invalid number of arguments [%d]\n", argc);
        printf("\t./tcp_server port_to_listen_on file_directory\n");
        return -1;
    }

    // get listener port
    char* port_to_listen_on = argv[1];

    // get file directory
    char* file_directory = argv[2];

    // server setup
    int listenfd;  // listener socket
    int connectedfd; // connected socket

    // initialize the cache
    Cache *cache = init_cache();

    // client information
    struct sockaddr_storage client_address;
    socklen_t client_size;
    char line[BUFFSIZE];
    ssize_t bytes_read;
    char clientName[BUFFSIZE];
    char clientPort[BUFFSIZE];

    // creates a passive socket to be used for accepting connections
    listenfd = getlistenfd(port_to_listen_on);

    // infinite loop to get connections
    while(1){
        // get size of client
        client_size = sizeof(client_address);

        printf("Attempting to connect...\n");

        // attempt to connect to a client
        connectedfd = accept(listenfd, (struct sockaddr*)&client_address, &client_size);

        // check for success
        if(connectedfd == -1){
            printf("ERROR: Unable to connect to client\n");
            continue;
        }

        printf("Connected\n");

        // gets information about client
        if (getnameinfo((struct sockaddr*)&client_address, client_size,
                clientName, BUFFSIZE, clientPort, BUFFSIZE, 0)!=0) {
            fprintf(stderr, "error getting name information about client\n");
            continue;
        } else {
            printf("accepted connection from %s:%s\n", clientName, clientPort);
        }

        // read the initial request from the current client
        char buffer[1024];
        int valread = read(connectedfd, buffer, 1024);
        if(valread < 0){
            close(connectedfd);
            printf("ERROR: Failed to read file request from client\n");
            continue;
        }

        // add newline for easy string processing
        buffer[valread] = '\0';

        // construct the absolute path to the file
        string file_delim = "/";
        string absolute_path = file_directory + file_delim + buffer;

        // check if file is within cache
        if(!check_cache(buffer, *cache)){
            // file not within cache

            // attempt to find file in directory structure
            ifstream client_file;
            client_file.open(absolute_path, ios::in | ios::binary);

            if(client_file.is_open()){
                // file exists
                // update cache with file
                if(!update_cache(buffer, client_file, *cache)){
                    // file is too large for cache, read and send file from disk directly
                    send_from_disk(connectedfd, client_file, absolute_path);

                    continue;
                }

            }else{
                // file does not exist
                printf("File [%s] does not exist.\n", buffer);

                // construct file DNE message
                string file_message_a = "File [";
                string file_message_b = "] does not exist.\n";
                string message = file_message_a + buffer + file_message_b;

                // send DNE message to client
                unsigned long long int response_size = message.size();

                if(send(connectedfd, &response_size, sizeof(response_size), 0) < 0){
                    printf("ERROR: Failed to send client size of DNE message\n");
                    client_file.close();
                    close(connectedfd);

                    continue;
                }

                if(send(connectedfd, message.c_str(), message.size(), 0) < 0){
                    printf("ERROR: Failed to send client DNE message\n");
                    client_file.close();
                    close(connectedfd);

                    continue;
                }

                client_file.close();
                close(connectedfd);

                continue;
            }

            client_file.close();
        }

        // file found within cache
        auto content_iter = cache->contents->find(buffer);
        auto size_iter = cache->sizes->find(buffer);

        if(content_iter == cache->contents->end() || size_iter == cache->sizes->end()){
            printf("ERROR: Cache contents & sizes detected out of sync\n");
            exit(-1);
        }

        // send the file from cache
        unsigned long long int response_size = size_iter->second;
        
        // send the cached file's size to client
        if(send(connectedfd, &response_size, sizeof(response_size), 0) < 0){
            printf("ERROR: Failed to send cached file's size\n");
            close(connectedfd);
        }

        // send the cached file contents to client        
        if(send(connectedfd, content_iter->second, size_iter->second, 0) < 0){
            printf("ERROR: Failed to send cached file's contents\n");
            close(connectedfd);
        }

        close(connectedfd);
    }
}

// read and send file directly from disk
bool send_from_disk(const int &client_socket, ifstream &client_file, string &absolute_path){
    cout << "Sending file directly from disk" << endl;

    // get the total size of the file
    unsigned long long int response_size = get_file_size(client_file);

    // send client the size of file
    if(send(client_socket, &response_size, sizeof(response_size), 0) < 0){
        printf("ERROR: Failed to send client disk file size\n");
        client_file.close();
        close(client_socket);
        
        return false;
    }

    // send the client the file contents
    char buffer[BUFFSIZE];
    while(client_file){
        client_file.read(reinterpret_cast<char*>(&buffer), BUFFSIZE);

        if(send(client_socket, buffer, client_file.gcount(), 0) < 0){
            printf("ERROR: Failed to send client disk file contents");
            client_file.close();
            close(client_socket);

            return false;
        }
    }

    client_file.close();
    close(client_socket);

    return true;
}

// initialize cache components
Cache* init_cache(){
    // malloc the cache struct
    Cache *cache = reinterpret_cast<Cache*>(malloc(sizeof(Cache)));
    if(cache == NULL){
        cout << "ERROR: Failed to allocated Cache struct" << endl;
        exit(-1);
    }

    // try to create the file contents map
    try{
        cache->contents = new map<string, char*>();
    }catch(bad_alloc &error){
        cerr << error.what() << endl;
        cout << "ERROR: Unable to allocate cache.contents initialization map\n" << endl;
        exit(-1);
    }

    // try to create the file sizes map
    try{
        cache->sizes = new map<string, int>();
    }catch(bad_alloc &error){
        cerr << error.what() << endl;
        cout << "ERROR: Unable to allocate cache.sizes initialization map\n" << endl;
        exit(-1);
    }

    // try to create the caches timing map for LRU implementation
    try{
        cache->times = new map<string, double>();
    }catch(bad_alloc &error){
        cerr << error.what() << endl;
        cout << "ERROR: Unable to allocate cache.times initialization map\n" << endl;
        exit(-1);
    }

    // initialize the cache size to empty
    cache->total_size = 0;

    // return the allocated cache
    return cache;
}


// returns true if cache contains the file, else false
bool check_cache(const string &file_name, Cache &cache){
    // check if cache contains our file
    auto content_iter = cache.contents->find(file_name);

    if(content_iter == cache.contents->end()){
        // if not found return false
        return false;
    }else{
        printf("Cache hit. File [%s] sent to the client\n", file_name.c_str());

        // get the end execution time
	    auto check_time = chrono::high_resolution_clock::now();

	    // cast the total execution time to milliseconds
	    double cur_time = chrono::duration_cast<chrono::milliseconds>(check_time-START_TIME).count();

        // find and update the last used time
        auto time_iter = cache.times->find(file_name);
        if(time_iter == cache.times->end()){
            printf("ERROR: Failed to update cached file's [%s] time\n", file_name.c_str());
            exit(-1);
        }

        time_iter->second = cur_time;

        // if found, return true
        return true;
    }
}

// returns true if file is added to cache, else false (too big)
bool update_cache(const string &file_name, ifstream &client_file, Cache &cache){
    int file_size = get_file_size(client_file);

    if(file_size > MAX_CACHE_SIZE){
        // file is too large for our cache
        printf("File [%s] is too large for cache\n", file_name.c_str());

        // indicate size limitation
        return false;
    }else if(cache.total_size + file_size > MAX_CACHE_SIZE){
        // remove files from cache until there is enough space available
        printf("Removing files for [%s] to be inserted into cache\n", file_name.c_str());

        // Remove files to allow space according to least recently used
        make_room_LRU(cache, file_size);
    }

    // insert file into our cache
    printf("Inserting [%s] into cache\n", file_name.c_str());

    // increment the total size of our cache
    cache.total_size+=file_size;

    try{
        // insert the file size into cache
        cache.sizes->insert(pair<string, int>(file_name, file_size));
    }catch(bad_alloc &){
        printf("ERROR: Unable to insert new file size into cache\n");
        exit(-1);
    }

    // read file contents
    char * cache_buf = (char*)malloc(sizeof(char)*file_size);
    if(cache_buf == NULL){
        printf("ERROR: Failed to allocate buffer for file reading\n");
        exit(-1);
    }

    client_file.read(reinterpret_cast<char*>(cache_buf), file_size);
    if(!client_file.good()){
        printf("ERROR: Failed to read file for cacheing\n");
        exit(-1);
    }

    try{
        // insert the file contents into cache
        cache.contents->insert(pair<string, char*>(file_name, cache_buf));
    }catch(bad_alloc &){
        printf("ERROR: Failed to insert new file contents into cache\n");
        exit(-1);
    }

    // get check time for file
    auto check_time = chrono::high_resolution_clock::now();

	// cast the total execution time to milliseconds
	double cur_time = chrono::duration_cast<chrono::milliseconds>(check_time-START_TIME).count();

    try{
        // insert the last used time
        cache.times->insert(pair<string, double>(file_name, cur_time));
    }catch(bad_alloc &){
        printf("ERROR: Failed to insert/update new file cache time into cache\n");
        exit(-1);
    }

    // notify of successful update
    return true;
}

// remove files from cache according to least recently used policy
int make_room_LRU(Cache &cache, int client_file_size){
    // vectors to track our key / value mapping
    vector<string> keys;
    vector<double> vals;
    
    // used to track our key/value pairs during sort
    vector<pair<double, string>> pairs;

    // number of keys in our map
    int num_keys;
    
    try{
        // initialize our vectors with keys and values
        for(auto time_iter = cache.times->begin(); time_iter != cache.times->end(); ++time_iter){
            keys.push_back(time_iter->first);
            vals.push_back(time_iter->second);
        }

        // get the number of elements in the cache
        num_keys = keys.size();

        // add all key/value pairs to our pair vector
        for(int i = 0; i < num_keys; ++i){
            pairs.push_back(make_pair(vals[i], keys[i]));
        }
    }catch(bad_alloc &){
        printf("ERROR: Failed to obtain all key/value pairs\n");
        exit(-1);
    }

    // sort in ascending order by first element of pair (time)
    sort(pairs.begin(), pairs.end());

    // calculate the total size needed to fit our new file
    int size_needed = cache.total_size + client_file_size - MAX_CACHE_SIZE;
    int count = 0;

    // find & remove files to remove according to LRU
    for(int i = 0; i < num_keys; ++i){
        // our stopping criteria
        if(size_needed <= 0){
            break;
        }

        // track the number of removed files
        count++;

        // find the file
        auto iter = cache.sizes->find(pairs[i].second);

        // ensure maps are synchronized in keys
        if(iter == cache.sizes->end()){
            printf("ERROR: Cache Sizes map does not contain cached file key [%s]\n", pairs[i].second.c_str());
            exit(-1);
        }

        // decrement the total size needed
        size_needed -= iter->second;

        // remove the file from cache
        remove_cache_file(pairs[i].second, cache);
    }

    return count;
}

// removes a file completely from cache
void remove_cache_file(string &file_name, Cache &cache){
    // find the value for each map using our key
    auto size_it = cache.sizes->find(file_name);
    auto time_it = cache.times->find(file_name);
    auto content_it = cache.contents->find(file_name);
    
    // ensure all maps contain the key
    if(size_it == cache.sizes->end() || time_it == cache.times->end() || content_it == cache.contents->end()){
        printf("ERROR: Unable to remove file from cache. Cache maps are out of synch\n");
        exit(-1);
    }

    // remove the size
    cache.total_size -= size_it->second;
    cache.sizes->erase(size_it);

    // remove the time
    cache.times->erase(time_it);

    // free the contents array
    free(content_it->second);

    // remove the contents
    cache.contents->erase(content_it);
}

// returns the file size
int get_file_size(ifstream &client_file){
    // seek to the end of the file
    client_file.seekg(0, ios::end);

    // get the size of the file in bytes
    int size = client_file.tellg();

    // clear the file's state
    client_file.clear();

    // seek to the start of the file
    client_file.seekg(0, ios::beg);

    // ensure file successfully reset
    if(!client_file.good()){
        printf("ERROR: Failed to reset file stream's status\n");
        exit(-1);
    }

    // return the size of the file
    return size;
}