//
//  networkServer.mm
//  Traffic Generator
//
//  Created by Mark Hanson on 10/9/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//

#include "networkServer.h"
#include "assert.h"
#include "errno.h"
#include <sys/select.h>
#include <netdb.h>
#include "arpa/inet.h"
#include "signal.h"

std::map< short int, serviceProviderInterface * > providerList;

static connectionMgr  * connMgr = NULL;

static pthread_once_t once = PTHREAD_ONCE_INIT;


void * beginThread(void * inData)
{
    connMgr->runLoop();
    return NULL;
}

// we are using a singleton so we have to be careful about how we allocate.
void allocateConnectionMgr()
{
 
    connectionMgr * tmp = NULL;
    std::cout << __func__ << "allocating the thread for the connection mgr" << std::endl;
    tmp = new connectionMgr;
    connMgr = tmp;
    pthread_t newThread = NULL;
    pthread_create(&newThread, (const pthread_attr_t *) NULL, beginThread, (void *) NULL);
    pthread_detach(newThread);
}

// singleton accessor
connectionMgr * theTradeGenConnectionMgr()
{
    if(!connMgr)
        pthread_once(&once,allocateConnectionMgr);
    return connMgr;
}

// allocate new connection
void *createNewConnection(void * inData)
{
    unsigned char serviceId;
    struct sockaddr t_sockaddr;
    char straddr[INET6_ADDRSTRLEN];
    straddr[0] = '\0';

    bzero(&t_sockaddr, sizeof(t_sockaddr));
    
    socklen_t t_sock_len = sizeof(t_sockaddr);


    /* who's on the other end? */
    getpeername(((connectionInfo *) inData)->sockData.sockfd, &t_sockaddr, &t_sock_len);
    
    if(t_sockaddr.sa_family == AF_INET)
    {
        struct sockaddr_in *in = (struct sockaddr_in*) &t_sockaddr;
        inet_ntop(t_sockaddr.sa_family, (const void *)&in->sin_addr, straddr,INET6_ADDRSTRLEN);
    }
    else if (t_sockaddr.sa_family == AF_INET6)
    {
        struct sockaddr_in6 *in = (struct sockaddr_in6 *) &t_sockaddr;
        inet_ntop(t_sockaddr.sa_family, (const void *)&in->sin6_addr, straddr,INET6_ADDRSTRLEN);  
    }
    
    std::cout << __func__ << " connection from " << straddr << " " << std::endl;

    
    /* figure out who the service provider is */
    read(((connectionInfo *) inData)->sockData.sockfd, (void *)&serviceId, 1);
    
    /* call em */
    theTradeGenConnectionMgr()->callServiceProvider((short int) serviceId, (connectionInfo *) inData);
    delete ((connectionInfo *) inData)->comms;
    delete ((connectionInfo *) inData);
    /* so we finished let the ui know we are letting the connection go */

    /* dump it from the connectin list. */
   // theTradeGenConnectionMgr()->remove(((struct sock_info *) inData)->sockfd);
    pthread_exit(NULL);
    return NULL;
}

int connectionMgr::init()
{
    return SUCCESS;
}

int connectionMgr::registerServiceProvider(short int serviceId, serviceProviderInterface *spi)
{
    if(providerList[serviceId] != NULL)
        return(ERROR);
    else
        providerList[serviceId] = spi;
    return SUCCESS;   
}

int connectionMgr::callServiceProvider(short int serviceId, connectionInfo * c_info)
{
    if(providerList[serviceId] == NULL)
        return(ERROR);
    else
        providerList[serviceId]->callback(&c_info->sockData, c_info->comms);
    return SUCCESS;    
}

int connectionMgr::unregisterServiceProvider(short int serviceId)
{
    if(providerList[serviceId] == NULL)
        return(ERROR);
    else
    {
        delete (providerList[serviceId]);
        providerList[serviceId] = NULL;
    }
    return SUCCESS;
}


connectionMgr::connectionMgr()
{
    lockHelper lock(&connectionMgrLock);
    m_comms = new ipc_comms_pipe();
    m_shutdown = false;
}

void connectionMgr::shutdownconnections()
{
    lockHelper lock(&connectionMgrLock);

    m_comms->signalShutdown();
    if(m_comms->shuttingDown() == SHUTTINGDOWN)
        std::cout << __FUNCTION__ << " : Shuttingdown Confirmed" << std::endl;
  //  m_shutdown = true;

}

int connectionMgr::runLoop(void)
{
      pthread_mutex_lock(&connectionMgrLock);
    
    int status = -1;
    
    /* Timeout for the accept */
    struct timeval timeout;
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;

    /*
     necessary stuff for getaddrinfo 
     AI_PASSIVE allows us to get our address settings
     */
    struct addrinfo hints;
    struct addrinfo *results = NULL, *res_head = NULL;
    
    bzero(&hints, sizeof(struct addrinfo));
    
    /* want to setup a tcp socket */
    hints.ai_flags = AI_PASSIVE;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_family = AF_UNSPEC;
    
    /* on port 3579 */
    status = getaddrinfo(NULL, "3579", &hints, &results);
    if(status < 0)
    {
        std::cout << " failed to get the addr info " << gai_strerror(errno) << std::endl;
        pthread_mutex_unlock(&connectionMgrLock);
        return ERROR;
    }
    
    /* if we have no results, there are real problems. */
    if(!results)
    {
        std::cout << " failed to get the results for socket " << std::endl;
        pthread_mutex_unlock(&connectionMgrLock);
        return ERROR;
    }
    
    /* want to count the results, we have at least one at this point */
    res_head = results;
    int count = 1;
    while(results->ai_next != NULL)
    {
        count++;
        results = results->ai_next;
    }
    
    /* setup and bind the socket with the settings we got back from getaddrinfo */
    m_socketServer = socket(res_head->ai_family, res_head->ai_socktype, res_head->ai_protocol);
    if(m_socketServer <= 0)
    {
        std::cout << __func__ << " failed to get the socket" << strerror(errno) << std::endl;
        pthread_mutex_unlock(&connectionMgrLock);
        return ERROR;
    }
    
    if(bind(m_socketServer, res_head->ai_addr, res_head->ai_addrlen) != 0)
    {
        std::cout << __func__ << " failed to bind the socket" << strerror(errno) << std::endl;
        close(m_socketServer);
        pthread_mutex_unlock(&connectionMgrLock);
        return ERROR;
    }
    
    int set = 1;
    setsockopt(m_socketServer, SOL_SOCKET, SO_NOSIGPIPE, (void *)&set, sizeof(int));
    
    std::cout << __func__ << " finished setting up the socket" << std::endl;
    
    status = listen(m_socketServer, MAX_CONNECTIONS);
    if(status != 0)
    {
        std::cout << __func__ << " listen failed " <<  strerror(errno) << "" << std::endl;
        pthread_mutex_unlock(&connectionMgrLock);
        return ERROR;
    }
    pthread_mutex_unlock(&connectionMgrLock);
    
    /* wait for connections indefinitely. */
    while(1)
    {
        std::cout << __func__ << "fish on!" << std::endl;
        try
        {
        for(int i = 0; i < MAX_CONNECTIONS; i++)
        {
            struct sockaddr socketClient_1;
            fd_set readfd;    
            socklen_t len = 0;
            int maxfd;
            struct sockaddr t_sockaddr;
            bzero(&t_sockaddr, sizeof(t_sockaddr));
            
            connectionInfo *temp = (connectionInfo *) malloc(sizeof(connectionInfo));
            temp->comms = new ipc_comms_pipe();

            FD_ZERO(&readfd);
            FD_SET(m_comms->myRead(), &readfd);
            FD_SET(m_socketServer, &readfd);
            if(m_comms->myRead() > m_socketServer)
                maxfd =  m_comms->myRead();
            else
                maxfd = m_socketServer;
            
           // std::cout << __FILE__ << "  " << __FUNCTION__ << " myRead " << m_comms->myRead() << " myWrite " << m_comms->myWrite() << " m_socketServer " << m_socketServer << " maxfd " << maxfd << std::endl;
            
          //  std::cout <<  __FILE__ << " " << __FUNCTION__ << " fd was " << maxfd<< std::endl; 
            switch (select(maxfd + 1, &readfd, NULL, NULL, NULL))
            {
                case 0:
                    return(SUCCESS);
                    break;
                case -1:
                    return (ERROR);
                    break;
                default:
                    if(FD_ISSET(m_socketServer, &readfd))
                    {
                        break;
                    }
                    else if (FD_ISSET(m_comms->myRead(), &readfd))
                    {
                        if(m_comms->timeToShutdown()== SHUTDOWN)
                        {
                            shutItAllDown();
                            m_comms->signalShuttingdown();
                            delete temp->comms;
                            free(temp);
                            continue;
                          //  return SUCCESS;
                        }
                        //std::cout << __FUNCTION__ << " " << "connection manager comms_read but not shutdown" << std::endl;
                        delete temp->comms;
                        free(temp);
                        continue;
                    }
                    break;
            }
            
            temp->sockData.sockfd = accept(m_socketServer, &socketClient_1, &len);
            
            if(temp->sockData.sockfd >= 0)
            {
                std::cout << __func__ << "accept completed." << std::endl;
                
                memcpy(&socketClient_1, &temp->sockData.s_addr, len);
                
                temp->sockData.len = len;
                add(temp->sockData.sockfd, temp->comms );
                
                                
                /* spawn a new thread */
                pthread_t client = NULL;
                pthread_create(&client, (pthread_attr_t *) NULL, createNewConnection, temp);
                pthread_detach(client);
            }
            else if( temp->sockData.sockfd == EINVAL )
            {
                sleep(5); // we are full up here. got to sleep
            }
            else 
            {
                std::cout << __func__ << " failed to accept the connection " << strerror(errno)<< std::endl;
            }
        }
        }
        catch(...)
        {
            std::cout << __FUNCTION__ << "Caught an error " << std::endl;
        }
    }
    
return(0);
}






