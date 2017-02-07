//
//  ipc_comms.h
//  tradegenerator
//
//  Created by Mark Hanson on 2/18/12.
//  Copyright 2012 __MyCompanyName__. All rights reserved.
//

#ifndef tradegenerator_ipc_comms_h
#define tradegenerator_ipc_comms_h
#include <iostream>

#include "st_defines.h"
extern "C"
{

#include "unistd.h"
#include "sys/select.h"    
}

#define SHUTDOWN (100)
#define SHUTTINGDOWN (101)

class ipc_comms_pipe
{
public:
    ipc_comms_pipe()
    {
        bzero(&toPipe, sizeof(int) *2);
        bzero(&fromPipe, sizeof(int) *2);
       if(pipe(toPipe) != 0)
            throw ERROR;
        if(pipe(fromPipe) != 0)
        {
            close(toPipe[0]);
            close(toPipe[1]);
            throw ERROR;
        }
    }
    ~ipc_comms_pipe()
    {}
    
    inline int myRead()
    {
        return fromPipe[0];
    }
    inline int myWrite()
    {
        return toPipe[1];
    }
    inline int serverRead()
    {
        return toPipe[0];
    }
    inline int serverWrite()
    {
        return fromPipe[1];
    }
    
    inline int signalShutdown()
    {
        std::cout <<  __FUNCTION__ << ": Shutdown requested" << std::endl; 
        return writeSignal(SHUTDOWN, serverWrite());    
    }
    
    inline int signalShuttingdown()
    {
        std::cout << __FUNCTION__ << ": Shuttingdown" << std::endl; 
        return writeSignal(SHUTTINGDOWN, myWrite());
    }
    
    inline int timeToShutdown()
    {
        return checkSignal(SHUTDOWN, myRead());        
    }
    
    inline int shuttingDown()
    {
        return checkSignal(SHUTTINGDOWN, serverRead());   
    }
protected:
    int checkSignal(int inSignal, int readPipefd);
    int writeSignal(int inSignal, int writePipefd);
private:
    int toPipe[2];
    int fromPipe[2];
};

#endif
