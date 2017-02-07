//
//  networkServer.h
//  Traffic Generator
//
//  Created by Mark Hanson on 10/9/11.
//  Copyright 2011 __MyCompanyName__. All rights reserved.
//
#ifndef NETWORK_SERVER_H
#define NETWORK_SERVER_H
#include <iostream>
#include "sys/socket.h"
#include "sys/types.h"
#include "netinet/in.h"
#include <vector>
#include <string>
#include <map>
#include <pthread.h>
extern "C"
{
#include "st_defines.h"
}
#include "ipc_comms.h"

class lockHelper
{
public:
    lockHelper(pthread_mutex_t * mutex)
    {
        m_mutex = mutex;
        pthread_mutex_lock(m_mutex);
    }
    ~lockHelper()
    {
        pthread_mutex_unlock(m_mutex);
    }
protected:
    pthread_mutex_t * m_mutex;
};


typedef enum {SU_add, SU_remove} SU_state;


typedef struct {
    int sockfd;
    struct sockaddr s_addr;
    socklen_t len;
} sock_info;

typedef struct
{
    sock_info sockData;
    ipc_comms_pipe * comms;
    
} connectionInfo;

class serviceProviderInterface
{
public:
    virtual int callback(sock_info * si, ipc_comms_pipe * comms) = 0;
    virtual ~serviceProviderInterface() {};
    
};


extern void * beginThread(void * inData);

#define MAX_CONNECTIONS (20)

extern void *createNewConnection(void * inData);
class connectionMgr
{
    
    friend connectionMgr * theTradeGenConnectionMgr();
    friend void allocateConnectionMgr();
    friend void * beginThread(void * inData);
    friend void *createNewConnection(void * inData);
protected:
    connectionMgr();
    std::map< int, ipc_comms_pipe * > connectionMap;
    pthread_t clients[MAX_CONNECTIONS];
    pthread_mutex_t connectionMgrLock;
    int init(); 
    bool m_shutdown;
    int m_socketServer;
    struct sockaddr_in6 m_sin;
    ipc_comms_pipe * m_comms;
    
public:
    ~connectionMgr();
    
    inline void add(int sockfd, ipc_comms_pipe * comms)
    {
        lockHelper lock(&connectionMgrLock);
        connectionMap[sockfd] = comms;
    }
    void shutdownconnections();
    int registerServiceProvider(short int serviceId, serviceProviderInterface *spi);
    
    
    int callServiceProvider(short int serviceId, connectionInfo * c_info);
    int unregisterServiceProvider(short int serviceId);
    inline int shutItAllDown()
    {
        //std::cout << __FUNCTION__ << " shutting it all down " << connectionMap.size() << " connections" << std::endl;
        for (std::map< int, ipc_comms_pipe * >::iterator i = connectionMap.begin(); i != connectionMap.end(); i++) {
            ((ipc_comms_pipe *)((*i).second))->signalShutdown();
            if(((ipc_comms_pipe *)((*i).second))->shuttingDown() != SHUTTINGDOWN)
                return (ERROR); 
        }
        return SUCCESS;
    }
    inline void remove(int sockfd)
    {
        
        lockHelper lock(&connectionMgrLock);
        connectionMap[sockfd] = NULL;
        
    }
    
    int runLoop(void);
    
    
    
    
};
extern connectionMgr * theTradeGenConnectionMgr();


#endif /* NETWORK_SERVER_H */





