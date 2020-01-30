# Network_Communication

# An overview which include the structure of your program

# The details of cache implementation


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