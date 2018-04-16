# Simple HTTP 1.0 Server
Project 1 for Computer System (COMP30023)

**Name** Zijun Chen  
**Student ID** 813190  
**Login Name** zijunc3  
***  

### Installation

    git clone git@gitlab.eng.unimelb.edu.au:zijunc3/comp30023-2018-project-1.git
    cd comp30023-2018-project-1/
    make

### Usage
     ./server [port number] [path to web root]

**Example**  

    ./server 8080 ./test/www/

### Debug Mode
1. Change
        #define DEBUG 0
    to
        #define DEBUG 1
    from file "debug_setting.h".

2. Recompile the program using

        make

### Extra Work
Other than the project requirement, I implemented:  
* Thread pool.
* Prevent directory traversal attack.
* Handle problems caused by the client closes the connection unexpectedly.
