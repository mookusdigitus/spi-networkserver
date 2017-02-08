//
//  tradeGenerationServer.h
//  multiServer
//
//  Created by Mark Hanson on 11/3/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#ifndef tradegenerator_tradeGenerationServer_h
#define tradegenerator_tradeGenerationServer_h
#include <iostream>
#include "networkServer.h"

typedef enum { C_DONE, C_READ, C_WRITE, C_ERROR, COMM_READ, COMM_WRITE } connection_wait;


class connection
{
public:
    connection(sock_info * si,  ipc_comms_pipe *comms)  
    {
        m_si = NULL;
        m_si = si;
        
        m_conn_comms = NULL;
        m_conn_comms = comms;
    }
    ~connection()
    {
        
    }
    connection_wait wait(struct timeval *timeout);
    int write(char *buffer, size_t len);
    int read();
    inline int close()
    {
        ::close(m_si->sockfd);
        std::cout << __FUNCTION__ << ": Closed connection" << std::endl;
        return SUCCESS;
    }
    int setup();
protected:
    sock_info * m_si;
    private:
    ipc_comms_pipe * m_conn_comms;
    fd_set readfd, writefd;
   
    
};

class multiServer : public virtual serviceProviderInterface
{

public:
    multiServer();
   ~multiServer()
    {
            //do nothing for now
    }
    int callback(sock_info * si, ipc_comms_pipe * comms);
    int getTimeSinceStart(timeval * diff);
protected:
    
private:
    timeval m_startTime;
    
};
#endif
