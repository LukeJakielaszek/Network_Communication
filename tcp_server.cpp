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

struct Cache{
    map<string, char*> *contents;
    map<string, int> *sizes;
    map<string, double> *times;
    int total_size;
};

char * retrieve_file(const string &file_name, const string &directory, Cache &cache);
bool update_cache(const string &file_name, ifstream &client_file, Cache &cache);
int get_file_size(ifstream &client_file);
Cache* init_cache();
bool check_cache(const string &file_name, Cache &cache);
bool send_from_disk(const int &client_socket, ifstream &client_file);
int make_room_LRU(Cache &cache, int client_file_size);
void remove_cache_file(string &file_name, Cache &cache);

// Buffer size for reading file
const int BUFFSIZE = 100;

// set max size to 64 MB
const int MAX_CACHE_SIZE = 64*1048576;

// start time of the program
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
        } else {
            printf("accepted connection from %s:%s\n", clientName, clientPort);
        }

        // read from the current client
        char buffer[1024];
        int valread = read(connectedfd, buffer, 1024);
        buffer[valread] = '\0';
        printf("[%d] : [%s]\n", valread, buffer);

        // construct the absolute path to the file
        string file_delim = "/";
        string absolute_path = file_directory + file_delim + buffer;

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
                    send_from_disk(connectedfd, client_file);

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
                send(connectedfd, message.c_str(), message.size(), 0);

                client_file.close();

                continue;
            }

            client_file.close();
        }

        // file found within cache
        auto content_iter = cache->contents->find(buffer);
        auto size_iter = cache->sizes->find(buffer);
        auto time_iter = cache->times->find(buffer);
        time_iter->second = 

        // send the file directly
        send(connectedfd, content_iter->second, size_iter->second, 0);
    }
}

// read and send file directly from disk
bool send_from_disk(const int &client_socket, ifstream &client_file){
    cout << "Sending file directly from disk" << endl;

    char buffer[BUFFSIZE];
    while(client_file){
        client_file.read(reinterpret_cast<char*>(&buffer), BUFFSIZE);

        send(client_socket, buffer, client_file.gcount(), 0);
    }

    client_file.close();

    return true;
}

// initialize cache components
Cache* init_cache(){
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

    // insert the file size into cache
    cache.sizes->insert(pair<string, int>(file_name, file_size));

    // read file contents
    char * cache_buf = (char*)malloc(sizeof(char)*file_size);
    client_file.read(reinterpret_cast<char*>(cache_buf), file_size);

    // insert the file contents into cache
    cache.contents->insert(pair<string, char*>(file_name, cache_buf));    

    // insert check time for file
    auto check_time = chrono::high_resolution_clock::now();

	// cast the total execution time to milliseconds
	double cur_time = chrono::duration_cast<chrono::milliseconds>(check_time-START_TIME).count();

    // insert the last used time
    cache.times->insert(pair<string, double>(file_name, cur_time));

    // notify of successful update
    return true;
}

// remove files from cache according to least recently used
int make_room_LRU(Cache &cache, int client_file_size){
    // vectors to track our key / value mapping
    vector<string> keys;
    vector<double> vals;

    // initialize our vectors with keys and values
    for(auto time_iter = cache.times->begin(); time_iter != cache.times->end(); ++time_iter){
        keys.push_back(time_iter->first);
        vals.push_back(time_iter->second);
    }

    // get the number of elements in the cache
    int num_keys = keys.size();

    // used to track our key/value pairs during sort
    vector<pair<double, string>> pairs;

    // add all key/value pairs to our pair vector
    for(int i = 0; i < num_keys; ++i){
        pairs.push_back(make_pair(vals[i], keys[i]));
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

        // decrement the total size needed
        size_needed -= iter->second;

        //remove the file from cache
        remove_cache_file(pairs[i].second, cache);
    }

    return count;
}

// removes a file completely from cache
void remove_cache_file(string &file_name, Cache &cache){
    // remove the size
    auto size_it = cache.sizes->find(file_name);
    cache.total_size -= size_it->second;
    cache.sizes->erase(size_it);

    // remove the time
    auto time_it = cache.times->find(file_name);
    cache.times->erase(time_it);

    // remove the contents
    auto content_it = cache.contents->find(file_name);
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

    // return the size of the file
    return size;
}