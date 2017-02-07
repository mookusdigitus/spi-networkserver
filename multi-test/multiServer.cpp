//
//  tradeGenerationServer.cpp
//  multiServer
//
//  Created by Mark Hanson on 11/3/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include <iostream>
#include <pthread.h>
#include <sstream>
#include "errno.h"
#include <vector>
#include "tradeGenerationServer.h"
#include "assert.h"
#include "sys/time.h"



/* parse the incoming request data. */

int connection::setup()
{
    char workingBuf[256] = {'\0'};
    size_t read_size = 0;
    
    
 //TODO: fix this
    if(::read(m_si->sockfd, (void *) &workingBuf, 1) <= 0)
    {
        std::cout << __FILE__ << ":" << __LINE__ << ": " << "read error" << std::endl;
        return ERROR;
    }
    
 /*
  something goes here.
  */
  if(::read(m_si->sockfd, (void *) workingBuf, read_size) != read_size)
    {
        
        std::cout << __FILE__ << ":" << __LINE__ << ": " << "read error" << std::endl;
        return ERROR;
    }
 /*
  something goes here
  */
    return SUCCESS;
}

/* non-blocking io wait */
connection_wait connection::wait(struct timeval *timeout)
{
    FD_ZERO(&readfd);
    FD_ZERO(&writefd);
    FD_SET(m_si->sockfd, &readfd);
    FD_SET(m_si->sockfd, &writefd);
    FD_SET(m_conn_comms->myRead(), &readfd);
    FD_SET(m_conn_comms->myWrite(), &writefd);
    int maxfd1, maxfd2;
    
    if(m_conn_comms->myRead() > m_conn_comms->myWrite())
        maxfd1 = m_conn_comms->myRead();
    else
        maxfd1 = m_conn_comms->myWrite();
    
    if(m_si->sockfd > maxfd1)
        maxfd2 = m_si->sockfd;
    else
        maxfd2 = maxfd1;
    
    switch (select(maxfd2+1, &readfd, &writefd, NULL, timeout))
    {
        case 0:
           // std::cout <<  __FILE__ << " " << __FUNCTION__ << ": timeout" << std::endl;
            return(C_DONE);
            break;
        case -1:
            std::cout <<  __FILE__ << " " << __FUNCTION__ << ": select error" << std::endl;
            return (C_ERROR);
            break;
        default:
            if(FD_ISSET(m_si->sockfd, &readfd))
            {
                //std::cout <<  __FILE__ << " " << __FUNCTION__ << ": data to read" << std::endl;
                return C_READ;
            }
            else if (FD_ISSET(m_conn_comms->myRead(), &readfd))
            {
               // std::cout << __FILE__ << " " << __FUNCTION__ << ": comm_read" << std::endl;
                return COMM_READ;
            }           
            else if (FD_ISSET(m_si->sockfd, &writefd))
            {
                //std::cout <<  __FILE__ << " " << __FUNCTION__ << ": can write" << std::endl;
                return C_WRITE;
            }

            else if (FD_ISSET(m_conn_comms->myWrite(), &writefd))
            {
                //std::cout << __FILE__ << " "<< __FUNCTION__ << ": comm_write" << std::endl;
                return COMM_WRITE;
            }
            break;
    }
    
    return C_ERROR;
}

/* write data across the tcp connection */
int connection::write(char *buffer, size_t len)
{
    ssize_t status = 0;
    status = ::write(m_si->sockfd, buffer, len);
    if(status != len)
    {
        std::cout << "connection::write error: " << strerror(errno) << std::endl;
        return ERROR;     
    }
    return SUCCESS;
}

int connection::read()
{
    char *buf = (char *) malloc(200);
    bzero(buf, 200);
    ::read(m_si->sockfd, buf, 200);
    
    std::cout << __FUNCTION__ << " read " << buf << std::endl;

    free(buf);
    return SUCCESS;
}

multiServer::multiServer()
{
    gettimeofday(&m_startTime, NULL);
}


int  multiServer::getTimeSinceStart(timeval * diff)
{
    timeval now = {0,0};
    gettimeofday(&now, NULL);
    long combined;
    diff->tv_sec = now.tv_sec - m_startTime.tv_sec;
    combined = diff->tv_sec*1000000 + (now.tv_usec - m_startTime.tv_usec);
    diff->tv_sec = combined/1000000;
    
    diff->tv_usec = combined % 1000000;
    return SUCCESS;
    
}


#define CB_CLEANUP     { \
delete inConnection; \
inConnection = NULL; \
} 
#define NOTIFY_AND_RETURN { \
std::cout << __FILE__ << ":" << __LINE__ << "error " << std::endl; \
return ERROR; \
}



/* this is the callback for the service provider. its job is to handle a tcp connection until close. */
int multiServer::callback(sock_info *si, ipc_comms_pipe * comms)
{
    bool done = false;
    int status = 0;
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    connection *inConnection;
    
    std::string quote;
    /* allocate a new connection class to manage the incoming connection */
    inConnection = new connection(si, comms); 
    if(inConnection->setup() == ERROR)
    {
        CB_CLEANUP
        NOTIFY_AND_RETURN
    }
    
    while (!done) 
    {
        /* give everyone a chance to get ready. */
     
                timeout.tv_sec=0;
                timeout.tv_usec = 0;
                status = ERROR;
        
                /* 
                 read the message queue here
                 */
 //TODO:               //status = generateTrade(m_exchange, quote, &timeout);
                if(status == SUCCESS)
                {
                    if(inConnection->write(quote) != SUCCESS)
                    {
                        std::cout << __FUNCTION__ << " failed to write to the connection" << std::endl;
                        CB_CLEANUP
                        NOTIFY_AND_RETURN
                    }
                }
                else
                    //wait.
                    continue;
                
                switch (inConnection->wait(&timeout))
            {
                case C_READ:
                    /* perhaps the remote is done check it */

                    inConnection->read();
                    break;
                case C_WRITE:
//TODO:                    inConnection->write(quote);
                    break;
                case C_ERROR:
                    CB_CLEANUP
                case COMM_READ:
                    if(comms->timeToShutdown() == SHUTDOWN)
                    {
                        comms->signalShuttingdown();
                        inConnection->close();
                        CB_CLEANUP
                        return SUCCESS;
                    }
                default:
                    break;
            }
                
                break;
    }
    CB_CLEANUP
    return SUCCESS;
}





