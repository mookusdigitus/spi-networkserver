//
//  ipc_comms.cpp
//  tradegenerator
//
//  Created by Mark Hanson on 2/18/12.
//  Copyright 2012 __MyCompanyName__. All rights reserved.
//

#include <iostream>
#include "ipc_comms.h"

int ipc_comms_pipe::checkSignal(int inSignal, int readPipefd)
{
    fd_set readfd;
    FD_ZERO(&readfd);
    int signal = 0;
    
    FD_SET(readPipefd, &readfd);
    
    switch (select(readPipefd+1, &readfd, NULL, NULL, NULL))
    {
        case 0:
            std::cout << __FUNCTION__ << ": Timeout" << std::endl;
            return(ERROR);
            break;
        case -1:
            std::cout << __FUNCTION__ << ": ERROR" << std::endl;

            return (ERROR);
            break;
        default:
            if(FD_ISSET(readPipefd, &readfd))
            {
                if(read(readPipefd, &signal, sizeof(signal)) == -1)
                {
                    std::cout << __FUNCTION__ << ": ERROR 2" << std::endl;
                    return ERROR;
                
                }
                    if(signal == inSignal)
                    {
                       // std::cout << __FUNCTION__ << ": got signal" << std::endl;

                        return inSignal;
                    }
            }
            break;
    }
    
    return ERROR;
}

int ipc_comms_pipe::writeSignal(int inSignal, int writePipefd)
{

    fd_set writefd;
    FD_ZERO(&writefd);
    
    FD_SET(writePipefd, &writefd);
    
    switch (select(writePipefd+1, NULL, &writefd, NULL, NULL))
    {
        case 0:
            return(ERROR);
            break;
        case -1:
            return (ERROR);
            break;
        default:
            if(FD_ISSET(writePipefd, &writefd))
            {
                if(write(writePipefd, &inSignal, sizeof(inSignal)) != sizeof(inSignal))
                    return ERROR;

            }
            break;
    }

   // std::cout << __FUNCTION__ << ": sent signal" << std::endl;
    return SUCCESS;
}
