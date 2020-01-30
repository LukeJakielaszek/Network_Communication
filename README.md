# Network_Communication

Compile:
    make -B

Run:
    ./tcp_server port_to_listen_on file_directory
    ./tcp_client server_host server_port file_name directory

# Overview
Client/Server Protocol:
    The client and server follow a predefined protocol for message transfer. The
    client connects to the server and sends the file_name to the server. The server
    responds with -9999 if the server determines that the file does not exist. If the
    file does exist, the server responds with the total length of the file. Upon
    receiving this value, the client can prepare for reading and ensure that it has received 
    the entire file during transfer (in case the server fails while sending the file 
    contents over the connection). After sending the size of the file, the server then sends
    the overall file contents. If the file was small enough to fit in the cache, the server 
    will send the full file in one message for efficiency. However, if the file is larger 
    than the cache, the server sends the file contents in buffered messages to the client.
    Finally, the client saves the received file to its user-defined output directory. All
    files are read as binary to avoid hitting unknown characters such as null.

Client:
    The client first connects to the server. After successfully connecting, the client
    reads a single long long int from the server. If the clients reads -9999, then the 
    requested file does not exist and the client closes the connection and ends. Otherwise,
    the client knows the file has been successfully found along with the exact size of the file.
    The client prepares a buffer for reading and then loops over the socket until the full file
    contents have been transferred. The client writes to its file during each iteration of the loop.

Server:
    The server first initializes its cache and sets up a listener socket for clients to connect to.
    During connection with a client, the server will only fail if a critical error occurs such as 
    the maps within the cache going out of synchronization or an allocation error. Errors such as 
    the client's file not existing will simply be reported to the client and the server will end
    its connection to the client and continue on to the next. After setting up the listener socket, 
    the server will accept a client. Next, the server will read the file_name from the client and
    construct the absolute path to the file. The server checks if the file is within the cache. If
    it is, it returns the file directly from cache according to the client/server protocol. If it is not,
    the server then checks if the file exists within the filesystem according to the absolute path. If
    it does not, the server notifies the client according to the client/server protocol. However, if it 
    does, the server determines the size of the file and if it can fit in the cache at all. If the file
    is larger than the entire cache size, the file is transferred to the client through buffered reads
    from disk. If the file can fit inside the cache without evicting files, it is simply added to the cache
    then sent to the client. If the file can fit inside the cache if files are evicted, then one-by-one,
    the last accessed cached files are evicted according to the least recently used replacement policy until
    there is space for the file. The file is then transmitted to the client. After transmitting the file 
    or a DNE message, the connection to the client is closed.

# Cache Implementation
The cache is simulated using the struct Cache. The cache consists of 
three maps and an overall total_size int. All maps within the cache map from
the file_name to the corresponding value types. The cache's contents map holds
mappings from files to their actual contents. The cache's sizes map holds mappings
from the files to their size (in bytes). The cache's times map holds mappings from
the files to their least recently accessed times. The times are updated for a file
every time a hit occurs in the cache or a file is added to the cache. The total_size 
variable tracks the cumulative size of all file contents within the cache. To ensure 
that exactly 64 MB of file data can be tracked by the cache, the total_size only
increments and decrements based on the char* that the cache's contents map
maps to. This cache implements the least recently used replacement policy using
the times component of the cache to identify the last accessed files. If the
new file is smaller than 64 MB, the already cached files are evicted until there 
is enough space to store the file.

# Test Cases
    Requesting an invalid file
    Requesting from a dead server
    Server/Connection fails during transmission of file
    Request file for the first time (empty cache)
        - This file is small enough to fit in the cache
        - This file is too large to fit in the cache
    Request same small file multiple times (cache hit)
    Request several small files multiple times (multiple cache hits)
    Request several small files until cache fills
        - request again with small file to eject 1 file from cache
        - request again with large file to eject multiple files from cache
    Request transfer of an executable file rather than plain text to ensure no loss of information
        - Note: The executable is successfully copied, but with only RW privileges.
        This executable can be ran after using the command chmod 777 filename to change
        the file permissions of the copied file.