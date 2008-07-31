///////////////////////////////////////////////////////////////////////////
//
// Copyright (c) 2000-2003 Intel Corporation 
// All rights reserved. 
//
// Redistribution and use in source and binary forms, with or without 
// modification, are permitted provided that the following conditions are met: 
//
// * Redistributions of source code must retain the above copyright notice, 
// this list of conditions and the following disclaimer. 
// * Redistributions in binary form must reproduce the above copyright notice, 
// this list of conditions and the following disclaimer in the documentation 
// and/or other materials provided with the distribution. 
// * Neither name of Intel Corporation nor the names of its contributors 
// may be used to endorse or promote products derived from this software 
// without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
// "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
// LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR 
// A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL INTEL OR 
// CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, 
// EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, 
// PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
// PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY 
// OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
// NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS 
// SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
//
///////////////////////////////////////////////////////////////////////////


#include "config.h"
#include <assert.h>
#include <signal.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#ifndef WIN32
	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>

	#ifndef SPARC_SOLARIS
//		#include <linux/if.h>
		#include <net/if.h>
	#else
		#include <fcntl.h>
		#include <net/if.h>
		#include <sys/sockio.h>
	#endif

	#include <sys/ioctl.h>
	#include <sys/utsname.h>
	#include <unistd.h>

	#include <sys/param.h>
	#if (defined(BSD) && BSD >= 199306)
		#include <ifaddrs.h>
	#endif
#endif
#include "upnpapi.h"
#include "httpreadwrite.h"
#include "ssdplib.h"
#include "soaplib.h"
#include "ThreadPool.h"
#include "membuffer.h"

#include "httpreadwrite.h"

// Needed for GENA
#include "gena.h"
#include "service_table.h"
#include "miniserver.h"

#ifdef INTERNAL_WEB_SERVER
	#include "webserver.h"
	#include "urlconfig.h"
#endif // INTERNAL_WEB_SERVER

virtualDirList *pVirtualDirList;

// Mutex to synchronize the subscription handling at the client side
CLIENTONLY( ithread_mutex_t GlobalClientSubscribeMutex; )

// rwlock to synchronize handles (root device or control point handle)
    ithread_rwlock_t GlobalHndRWLock;

// Mutex to synchronize the uuid creation process
    ithread_mutex_t gUUIDMutex;

    TimerThread gTimerThread;

    ThreadPool gSendThreadPool;
    ThreadPool gRecvThreadPool;
    ThreadPool gMiniServerThreadPool;

//Flag to indicate the state of web server
     WebServerState bWebServerState = WEB_SERVER_DISABLED;

// static buffer to store the local host ip address or host name
     char LOCAL_HOST[LINE_SIZE];

// local port for the mini-server
     unsigned short LOCAL_PORT;

// UPnP device and control point handle table 
     void *HandleTable[NUM_HANDLE];

//This structure is for virtual directory callbacks
     struct dlnaVirtualDirCallbacks virtualDirCallback;

// a local dir which serves as webserver root
     extern membuffer gDocumentRootDir;

// Maximum content-length that the SDK will process on an incoming packet. 
// Content-Length exceeding this size will be not processed and error 413 
// (HTTP Error Code) will be returned to the remote end point.
size_t g_maxContentLength = DEFAULT_SOAP_CONTENT_LENGTH; // in bytes

// Global variable to denote the state of dlna SDK 
//    = 0 if uninitialized, = 1 if initialized.
     int dlnaSdkInit = 0;

// Global variable to denote the state of dlna SDK device registration.
// = 0 if unregistered, = 1 if registered.
     int dlnaSdkDeviceRegistered = 0;

// Global variable to denote the state of dlna SDK client registration.
// = 0 if unregistered, = 1 if registered.
     int dlnaSdkClientRegistered = 0;

/****************************************************************************
 * Function: dlnaInit
 *
 * Parameters:		
 *	IN const char * HostIP: Local IP Address
 *	IN short DestPort: Local Port to listen for incoming connections
 * Description:
 *	Initializes 
 *		- Mutex objects, 
 *		- Handle Table
 *		- Thread Pool and Thread Pool Attributes
 *		- MiniServer(starts listening for incoming requests) 
 *			and WebServer (Sends request to the 
 *		        Upper Layer after HTTP Parsing)
 *		- Checks for IP Address passed as an argument. IF NULL, 
 *                gets local host name
 *		- Sets GENA and SOAP Callbacks.
 *		- Starts the timer thread.
 *
 * Returns:
 *	DLNA_E_SUCCESS on success, nonzero on failure.
 *	DLNA_E_INIT_FAILED if Initialization fails.
 *	DLNA_E_INIT if UPnP is already initialized
 *****************************************************************************/
int dlnaInit( IN const char *HostIP,
              IN unsigned short DestPort )
{
    int retVal = 0;
    ThreadPoolAttr attr;
#ifdef WIN32
	WORD wVersionRequested;
	WSADATA wsaData;
	int err;
#endif

    if( dlnaSdkInit == 1 ) {
        // already initialized
        return DLNA_E_INIT;
    }

#ifdef WIN32
	wVersionRequested = MAKEWORD( 2, 2 );

	err = WSAStartup( wVersionRequested, &wsaData );
	if ( err != 0 ) {
		/* Tell the user that we could not find a usable */
		/* WinSock DLL.                                  */
		return DLNA_E_INIT_FAILED;
	}

	/* Confirm that the WinSock DLL supports 2.2.*/
	/* Note that if the DLL supports versions greater    */
	/* than 2.2 in addition to 2.2, it will still return */
	/* 2.2 in wVersion since that is the version we      */
	/* requested.                                        */
	 
	if ( LOBYTE( wsaData.wVersion ) != 2 ||
			HIBYTE( wsaData.wVersion ) != 2 ) {
		/* Tell the user that we could not find a usable */
		/* WinSock DLL.                                  */
		WSACleanup( );
		return DLNA_E_INIT_FAILED; 
	}

	/* The WinSock DLL is acceptable. Proceed. */
#endif

    membuffer_init( &gDocumentRootDir );

    srand( time( NULL ) );      // needed by SSDP or other parts

    if( dlnaInitLog() != DLNA_E_SUCCESS ) {
             return DLNA_E_INIT_FAILED;
    }

    dlnaPrintf( DLNA_INFO, API, __FILE__, __LINE__, "Inside dlnaInit \n" );
    // initialize mutex
#ifdef __CYGWIN__
        /* On Cygwin, pthread_mutex_init() fails without this memset. */
        /* TODO: Fix Cygwin so we don't need this memset(). */
        memset(&GlobalHndRWLock, 0, sizeof(GlobalHndRWLock));
#endif
    if (ithread_rwlock_init(&GlobalHndRWLock, NULL) != 0) {
        return DLNA_E_INIT_FAILED;
    }

    if (ithread_mutex_init(&gUUIDMutex, NULL) != 0) {
        return DLNA_E_INIT_FAILED;
    }
    // initialize subscribe mutex
#ifdef INCLUDE_CLIENT_APIS
    if (ithread_mutex_init(&GlobalClientSubscribeMutex, NULL) != 0) {
        return DLNA_E_INIT_FAILED;
    }
#endif

    HandleLock();
    if( HostIP != NULL ) {
        strcpy( LOCAL_HOST, HostIP );
    } else {
        if( getlocalhostname( LOCAL_HOST ) != DLNA_E_SUCCESS ) {
            HandleUnlock();
            return DLNA_E_INIT_FAILED;
        }
    }

    if( dlnaSdkInit != 0 ) {
        HandleUnlock();
        return DLNA_E_INIT;
    }

    InitHandleList();
    HandleUnlock();

    TPAttrInit( &attr );
    TPAttrSetMaxThreads( &attr, MAX_THREADS );
    TPAttrSetMinThreads( &attr, MIN_THREADS );
    TPAttrSetJobsPerThread( &attr, JOBS_PER_THREAD );
    TPAttrSetIdleTime( &attr, THREAD_IDLE_TIME );
    TPAttrSetMaxJobsTotal( &attr, MAX_JOBS_TOTAL );

    if( ThreadPoolInit( &gSendThreadPool, &attr ) != DLNA_E_SUCCESS ) {
        dlnaSdkInit = 0;
        dlnaFinish();
        return DLNA_E_INIT_FAILED;
    }

    if( ThreadPoolInit( &gRecvThreadPool, &attr ) != DLNA_E_SUCCESS ) {
        dlnaSdkInit = 0;
        dlnaFinish();
        return DLNA_E_INIT_FAILED;
    }

    TPAttrSetMinThreads( &attr, 4 );
    
    if( ThreadPoolInit( &gMiniServerThreadPool, &attr ) != DLNA_E_SUCCESS ) {
        dlnaSdkInit = 0;
        dlnaFinish();
        return DLNA_E_INIT_FAILED;
    }

    dlnaSdkInit = 1;
#if EXCLUDE_SOAP == 0
    SetSoapCallback( soap_device_callback );
#endif
#if EXCLUDE_GENA == 0
    SetGenaCallback( genaCallback );
#endif

    if( ( retVal = TimerThreadInit( &gTimerThread,
                                    &gSendThreadPool ) ) !=
        DLNA_E_SUCCESS ) {
        dlnaSdkInit = 0;
        dlnaFinish();
        return retVal;
    }
#if EXCLUDE_MINISERVER == 0
    if( ( retVal = StartMiniServer( DestPort ) ) <= 0 ) {
        dlnaPrintf( DLNA_CRITICAL, API, __FILE__, __LINE__,
            "Miniserver failed to start" );
        dlnaFinish();
        dlnaSdkInit = 0;
        if( retVal != -1 )
            return retVal;
        else                    // if miniserver is already running for unknown reasons!
            return DLNA_E_INIT_FAILED;
    }
#endif
    DestPort = retVal;
    LOCAL_PORT = DestPort;

#if EXCLUDE_WEB_SERVER == 0
    if( ( retVal =
          dlnaEnableWebserver( WEB_SERVER_ENABLED ) ) != DLNA_E_SUCCESS ) {
        dlnaFinish();
        dlnaSdkInit = 0;
        return retVal;
    }
#endif

    dlnaPrintf( DLNA_INFO, API, __FILE__, __LINE__,
        "Host Ip: %s Host Port: %d\n", LOCAL_HOST,
        LOCAL_PORT );

    dlnaPrintf( DLNA_INFO, API, __FILE__, __LINE__, "Exiting dlnaInit \n" );

    return DLNA_E_SUCCESS;

} /***************** end of dlnaInit ******************/

#ifdef DEBUG
static void 
PrintThreadPoolStats(
	ThreadPool *tp, 
	const char *DbgFileName,
	int DbgLineNo,
	const char *msg)
{
	ThreadPoolStats stats;
	ThreadPoolGetStats(tp, &stats);
	dlnaPrintf(DLNA_INFO, API, DbgFileName, DbgLineNo, 
		"%s \n"
		"High Jobs pending: %d\n"
		"Med Jobs Pending: %d\n"
		"Low Jobs Pending: %d\n"
		"Average wait in High Q in milliseconds: %lf\n"
		"Average wait in Med Q in milliseconds: %lf\n"
		"Average wait in Low Q in milliseconds: %lf\n"
		"Max Threads Used: %d\n"
		"Worker Threads: %d\n"
		"Persistent Threads: %d\n"
		"Idle Threads: %d\n"
		"Total Threads: %d\n"
		"Total Work Time: %lf\n"
		"Total Idle Time: %lf\n",
		msg,
		stats.currentJobsHQ,
		stats.currentJobsMQ,
		stats.currentJobsLQ,
		stats.avgWaitHQ,
		stats.avgWaitMQ,
		stats.avgWaitLQ,
		stats.maxThreads,
		stats.workerThreads,
		stats.persistentThreads,
		stats.idleThreads,
		stats.totalThreads,
		stats.totalWorkTime,
		stats.totalIdleTime);
}
#else /* DEBUG */
static DLNA_INLINE void 
PrintThreadPoolStats(
	ThreadPool *tp dlna_unused, 
	const char *DbgFileName dlna_unused,
	int DbgLineNo dlna_unused,
	const char *msg dlna_unused)
{
}
#endif /* DEBUG */


/****************************************************************************
 * Function: dlnaFinish
 *
 * Parameters:	NONE
 *
 * Description:
 *	Checks for pending jobs and threads 
 *		Unregisters either the client or device 
 *		Shuts down the Timer Thread
 *		Stops the Mini Server
 *		Uninitializes the Thread Pool
 *		For Win32 cleans up Winsock Interface 
 *		Cleans up mutex objects
 *
 * Return Values:
 *	DLNA_E_SUCCESS on success, nonzero on failure.
 *****************************************************************************/
int
dlnaFinish()
{
#ifdef INCLUDE_DEVICE_APIS
    dlnaDevice_Handle device_handle;
#endif
#ifdef INCLUDE_CLIENT_APIS
    dlnaClient_Handle client_handle;
#endif
    struct Handle_Info *temp;

#ifdef WIN32
//	WSACleanup();
#endif

    if( dlnaSdkInit != 1 ) {
        return DLNA_E_FINISH;
    }

    dlnaPrintf( DLNA_INFO, API, __FILE__, __LINE__,
        "Inside dlnaFinish : dlnaSdkInit is :%d:\n", dlnaSdkInit );
    if( dlnaSdkInit == 1 ) {
        dlnaPrintf( DLNA_INFO, API, __FILE__, __LINE__,
            "dlnaFinish : dlnaSdkInit is ONE\n" );
    }
    PrintThreadPoolStats(&gSendThreadPool, __FILE__, __LINE__, "Send Thread Pool");
    PrintThreadPoolStats(&gRecvThreadPool, __FILE__, __LINE__, "Recv Thread Pool");
    PrintThreadPoolStats(&gMiniServerThreadPool, __FILE__, __LINE__, "MiniServer Thread Pool");

#ifdef INCLUDE_DEVICE_APIS
    if( GetDeviceHandleInfo( &device_handle, &temp ) == HND_DEVICE )
        dlnaUnRegisterRootDevice( device_handle );
#endif

#ifdef INCLUDE_CLIENT_APIS
    if( GetClientHandleInfo( &client_handle, &temp ) == HND_CLIENT )
        dlnaUnRegisterClient( client_handle );
#endif

    TimerThreadShutdown( &gTimerThread );
    StopMiniServer();

#if EXCLUDE_WEB_SERVER == 0
    web_server_destroy();
#endif

    ThreadPoolShutdown(&gMiniServerThreadPool);
    ThreadPoolShutdown(&gRecvThreadPool);
    ThreadPoolShutdown(&gSendThreadPool);

    PrintThreadPoolStats(&gSendThreadPool, __FILE__, __LINE__, "Send Thread Pool");
    PrintThreadPoolStats(&gRecvThreadPool, __FILE__, __LINE__, "Recv Thread Pool");
    PrintThreadPoolStats(&gMiniServerThreadPool, __FILE__, __LINE__, "MiniServer Thread Pool");

#ifdef INCLUDE_CLIENT_APIS
    ithread_mutex_destroy(&GlobalClientSubscribeMutex);
#endif
    ithread_rwlock_destroy(&GlobalHndRWLock);
    ithread_mutex_destroy(&gUUIDMutex);

    // remove all virtual dirs
    dlnaRemoveAllVirtualDirs();

    // allow static linking
#ifdef WIN32
#ifdef PTW32_STATIC_LIB
    pthread_win32_thread_detach_np();
#endif
#endif

    dlnaSdkInit = 0;
    dlnaPrintf( DLNA_INFO, API, __FILE__, __LINE__,
        "Exiting dlnaFinish : dlnaSdkInit is :%d:\n", dlnaSdkInit);
    dlnaCloseLog();

    return DLNA_E_SUCCESS;

}
/*************************** End of  dlnaFinish  *****************************/

/******************************************************************************
 * Function: dlnaGetServerPort
 *
 * Parameters: NONE
 *
 * Description:
 *	Gives back the miniserver port.
 *
 * Return Values:
 *	local port on success, zero on failure.
 *****************************************************************************/
unsigned short
dlnaGetServerPort( void )
{

    if( dlnaSdkInit != 1 )
        return 0;

    return LOCAL_PORT;
}

/***************************************************************************
 * Function: dlnaGetServerIpAddress
 *
 * Parameters: NONE
 *
 * Description:
 *	Gives back the local ipaddress.
 *
 * Return Values: char *
 *	return the IP address string on success else NULL of failure
 ***************************************************************************/
char *
dlnaGetServerIpAddress( void )
{

    if( dlnaSdkInit != 1 )
        return NULL;

    return LOCAL_HOST;
}

#ifdef INCLUDE_DEVICE_APIS
#if 0

/****************************************************************************
 * Function: dlnaAddRootDevice
 *
 * Parameters:	
 *	IN const char *DescURL: Location of the root device 
 *		description xml file
 *	IN dlnaDevice_Handle Hnd: The device handle
 *
 * Description:
 *	downloads the description file and update the service table of the
 *	device. This function has been deprecated.
 *
 * Return Values:
 *	DLNA_E_SUCCESS on success, nonzero on failure.
 *****************************************************************************/
int
dlnaAddRootDevice( IN const char *DescURL,
                   IN dlnaDevice_Handle Hnd )
{
    int retVal = 0;
    struct Handle_Info *HInfo;
    IXML_Document *temp;

    if( dlnaSdkInit != 1 ) {
        return DLNA_E_FINISH;
    }

    if( ( retVal =
          dlnaDownloadXmlDoc( DescURL, &( temp ) ) ) != DLNA_E_SUCCESS ) {
        return retVal;
    }

    HandleLock();
    if( GetHandleInfo( Hnd, &HInfo ) == DLNA_E_INVALID_HANDLE ) {
        HandleUnlock();
        ixmlDocument_free( temp );
        return DLNA_E_INVALID_HANDLE;
    }

    if( addServiceTable
        ( ( IXML_Node * ) temp, &HInfo->ServiceTable, DescURL ) ) {

        dlnaPrintf( DLNA_INFO, API, __FILE__, __LINE__,
            "dlnaAddRootDevice: GENA Service Table \n" );
        dlnaPrintf( DLNA_INFO, API, __FILE__, __LINE__,
            "Here are the known services: \n" );
        printServiceTable( &HInfo->ServiceTable, DLNA_INFO, API );
    } else {
        dlnaPrintf( DLNA_INFO, API, __FILE__, __LINE__,
            "\ndlnaAddRootDevice: No Eventing Support Found \n" );
    }

    ixmlDocument_free( temp );
    HandleUnlock();

    return DLNA_E_SUCCESS;
}
#endif // 0
#endif //INCLUDE_DEVICE_APIS

#ifdef INCLUDE_DEVICE_APIS

/****************************************************************************
 * Function: dlnaRegisterRootDevice
 *
 * Parameters:	
 *	IN const char *DescUrl:Pointer to a string containing the 
 *		description URL for this root device instance. 
 *	IN dlna_FunPtr Callback: Pointer to the callback function for 
 *		receiving asynchronous events. 
 *	IN const void *Cookie: Pointer to user data returned with the 
 *		callback function when invoked.
 *	OUT dlnaDevice_Handle *Hnd: Pointer to a variable to store the 
 *		new device handle.
 *
 * Description:
 *	This function registers a device application with
 *	the UPnP Library.  A device application cannot make any other API
 *	calls until it registers using this function.  
 *
 * Return Values:
 *	DLNA_E_SUCCESS on success, nonzero on failure.
 *****************************************************************************/
int
dlnaRegisterRootDevice( IN const char *DescUrl,
                        IN dlna_FunPtr Fun,
                        IN const void *Cookie,
                        OUT dlnaDevice_Handle * Hnd )
{

    struct Handle_Info *HInfo;
    int retVal = 0;

    if( dlnaSdkInit != 1 ) {
        return DLNA_E_FINISH;
    }

    dlnaPrintf( DLNA_INFO, API, __FILE__, __LINE__,
        "Inside dlnaRegisterRootDevice\n" );
    
    HandleLock();
    if( dlnaSdkDeviceRegistered ) {
        HandleUnlock();
        return DLNA_E_ALREADY_REGISTERED;
    }

    if( Hnd == NULL || Fun == NULL ||
        DescUrl == NULL || strlen( DescUrl ) == 0 ) {
        HandleUnlock();
        return DLNA_E_INVALID_PARAM;
    }

    if( ( *Hnd = GetFreeHandle() ) == DLNA_E_OUTOF_HANDLE ) {
        HandleUnlock();
        return DLNA_E_OUTOF_MEMORY;
    }

    HInfo = ( struct Handle_Info * )malloc( sizeof( struct Handle_Info ) );
    if( HInfo == NULL ) {
        HandleUnlock();
        return DLNA_E_OUTOF_MEMORY;
    }
    HandleTable[*Hnd] = HInfo;

    dlnaPrintf( DLNA_INFO, API, __FILE__, __LINE__,
        "Root device URL is %s\n", DescUrl );

    HInfo->aliasInstalled = 0;
    HInfo->HType = HND_DEVICE;
    strcpy( HInfo->DescURL, DescUrl );
    HInfo->Callback = Fun;
    HInfo->Cookie = ( void * )Cookie;
    HInfo->MaxAge = DEFAULT_MAXAGE;
    HInfo->DeviceList = NULL;
    HInfo->ServiceList = NULL;
    HInfo->DescDocument = NULL;
    CLIENTONLY( ListInit( &HInfo->SsdpSearchList, NULL, NULL ); )
    CLIENTONLY( HInfo->ClientSubList = NULL; )
    HInfo->MaxSubscriptions = DLNA_INFINITE;
    HInfo->MaxSubscriptionTimeOut = DLNA_INFINITE;

    if( ( retVal =
          dlnaDownloadXmlDoc( HInfo->DescURL, &( HInfo->DescDocument ) ) )
        != DLNA_E_SUCCESS ) {
        CLIENTONLY( ListDestroy( &HInfo->SsdpSearchList, 0 ) );
        FreeHandle( *Hnd );
        HandleUnlock();
        return retVal;
    }

    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "dlnaRegisterRootDevice: Valid Description\n" );
    dlnaPrintf( DLNA_INFO, API, __FILE__, __LINE__,
        "dlnaRegisterRootDevice: DescURL : %s\n",
        HInfo->DescURL );

    HInfo->DeviceList =
        ixmlDocument_getElementsByTagName( HInfo->DescDocument, "device" );
    if( !HInfo->DeviceList ) {
        CLIENTONLY( ListDestroy( &HInfo->SsdpSearchList, 0 ) );
        ixmlDocument_free( HInfo->DescDocument );
        FreeHandle( *Hnd );
        HandleUnlock();
        dlnaPrintf( DLNA_CRITICAL, API, __FILE__, __LINE__,
            "dlnaRegisterRootDevice: No devices found for RootDevice\n" );
        return DLNA_E_INVALID_DESC;
    }

    HInfo->ServiceList = ixmlDocument_getElementsByTagName(
        HInfo->DescDocument, "serviceList" );
    if( !HInfo->ServiceList ) {
        dlnaPrintf( DLNA_CRITICAL, API, __FILE__, __LINE__,
            "dlnaRegisterRootDevice: No services found for RootDevice\n" );
    }

    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "dlnaRegisterRootDevice: Gena Check\n" );
    //*******************************
    // GENA SET UP
    //*******************************
    if( getServiceTable( ( IXML_Node * ) HInfo->DescDocument,
            &HInfo->ServiceTable, HInfo->DescURL ) ) {
        dlnaPrintf( DLNA_INFO, API, __FILE__, __LINE__,
            "dlnaRegisterRootDevice: GENA Service Table \n" );
        dlnaPrintf( DLNA_INFO, API, __FILE__, __LINE__,
            "Here are the known services: \n" );
        printServiceTable( &HInfo->ServiceTable, DLNA_INFO, API );
    } else {
        dlnaPrintf( DLNA_INFO, API, __FILE__, __LINE__,
            "\ndlnaRegisterRootDevice2: Empty service table\n" );
    }

    dlnaSdkDeviceRegistered = 1;
    HandleUnlock();
    dlnaPrintf( DLNA_INFO, API, __FILE__, __LINE__,
        "Exiting RegisterRootDevice Successfully\n" );

    return DLNA_E_SUCCESS;
}

#endif // INCLUDE_DEVICE_APIS

#ifdef INCLUDE_DEVICE_APIS
#if 0

/****************************************************************************
 * Function: dlnaRemoveRootDevice
 *
 * Parameters:	
 *	IN const char *DescURL: Location of the root device 
 *		description xml file
 *	IN dlnaDevice_Handle Hnd: The device handle
 *
 * Description:
 *	downloads the description file and update the service table of the
 *	device. This function has been deprecated.
 *
 * Return Values:
 *	DLNA_E_SUCCESS on success, nonzero on failure.
 *****************************************************************************/
int
dlnaRemoveRootDevice( IN const char *DescURL,
                      IN dlnaDevice_Handle Hnd )
{
    int retVal = 0;
    struct Handle_Info *HInfo;

    IXML_Document *temp;

    if( dlnaSdkInit != 1 ) {
        return DLNA_E_FINISH;
    }

    if( ( retVal =
          dlnaDownloadXmlDoc( DescURL, &( temp ) ) ) != DLNA_E_SUCCESS ) {
        return retVal;
    }

    HandleLock();
    if( GetHandleInfo( Hnd, &HInfo ) == DLNA_E_INVALID_HANDLE ) {
        HandleUnlock();
        ixmlDocument_free( temp );
        return DLNA_E_INVALID_HANDLE;
    }

    if( removeServiceTable( ( IXML_Node * ) temp, &HInfo->ServiceTable ) ) {

        dlnaPrintf( DLNA_INFO, API, __FILE__, __LINE__,
            "dlnaRemoveRootDevice: GENA Service Table \n" );
        dlnaPrintf( DLNA_INFO, API, __FILE__, __LINE__,
            "Here are the known services: \n" );
        printServiceTable( &HInfo->ServiceTable, DLNA_INFO, API );
    } else {
        dlnaPrintf( DLNA_INFO, API, __FILE__, __LINE__,
            "\ndlnaRemoveRootDevice: No Services Removed\n" );
        dlnaPrintf( DLNA_INFO, API, __FILE__, __LINE__,
            "Here are the known services: \n" );
        printServiceTable( &HInfo->ServiceTable, DLNA_INFO, API );
    }

    HandleUnlock();

    ixmlDocument_free( temp );
    return DLNA_E_SUCCESS;
}
#endif //0
#endif //INCLUDE_DEVICE_APIS

#ifdef INCLUDE_DEVICE_APIS

/****************************************************************************
 * Function: dlnaUnRegisterRootDevice
 *
 * Parameters:	
 *	IN dlnaDevice_Handle Hnd: The handle of the device instance 
 *		to unregister
 * Description:
 *	This function unregisters a root device registered with 
 *	dlnaRegisterRootDevice} or dlnaRegisterRootDevice2. After this call, the 
 *	dlnaDevice_Handle Hnd is no longer valid. For all advertisements that 
 *	have not yet expired, the UPnP library sends a device unavailable message 
 *	automatically. 
 *
 * Return Values:
 *	DLNA_E_SUCCESS on success, nonzero on failure.
 *****************************************************************************/
int
dlnaUnRegisterRootDevice( IN dlnaDevice_Handle Hnd )
{
    int retVal = 0;
    struct Handle_Info *HInfo = NULL;

    // struct Handle_Info *info=NULL;

    if( dlnaSdkInit != 1 ) {
        return DLNA_E_FINISH;
    }

    HandleLock();
    if( !dlnaSdkDeviceRegistered ) {
        HandleUnlock();
        return DLNA_E_INVALID_HANDLE;
    }
    HandleUnlock();

    dlnaPrintf( DLNA_INFO, API, __FILE__, __LINE__,
        "Inside dlnaUnRegisterRootDevice \n" );
#if EXCLUDE_GENA == 0
    if( genaUnregisterDevice( Hnd ) != DLNA_E_SUCCESS )
        return DLNA_E_INVALID_HANDLE;
#endif

    HandleLock();
    if( GetHandleInfo( Hnd, &HInfo ) == DLNA_E_INVALID_HANDLE ) {
        HandleUnlock();
        return DLNA_E_INVALID_HANDLE;
    }
    HandleUnlock();

#if EXCLUDE_SSDP == 0
    retVal = AdvertiseAndReply( -1, Hnd, 0, ( struct sockaddr_in * )NULL,
                                ( char * )NULL, ( char * )NULL,
                                ( char * )NULL, HInfo->MaxAge );
#endif

    HandleLock();
    if( GetHandleInfo( Hnd, &HInfo ) == DLNA_E_INVALID_HANDLE ) {
        HandleUnlock();
        return DLNA_E_INVALID_HANDLE;
    }
    //info = (struct Handle_Info *) HandleTable[Hnd];
    ixmlNodeList_free( HInfo->DeviceList );
    ixmlNodeList_free( HInfo->ServiceList );
    ixmlDocument_free( HInfo->DescDocument );

    CLIENTONLY( ListDestroy( &HInfo->SsdpSearchList, 0 ); )

#ifdef INTERNAL_WEB_SERVER
    if( HInfo->aliasInstalled ) {
        web_server_set_alias( NULL, NULL, 0, 0 );
    }
#endif // INTERNAL_WEB_SERVER

    FreeHandle( Hnd );
    dlnaSdkDeviceRegistered = 0;
    HandleUnlock();

    dlnaPrintf( DLNA_INFO, API, __FILE__, __LINE__,
        "Exiting dlnaUnRegisterRootDevice \n" );

    return retVal;

}  /****************** End of dlnaUnRegisterRootDevice *********************/

#endif //INCLUDE_DEVICE_APIS

// *************************************************************
#ifdef INCLUDE_DEVICE_APIS
#ifdef INTERNAL_WEB_SERVER

/**************************************************************************
 * Function: GetNameForAlias
 *
 * Parameters:	
 *	IN char *name: name of the file
 *	OUT char** alias: pointer to alias string 
 *
 * Description:
 *	This function determines alias for given name which is a file name 
 *	or URL.
 *
 * Return Values:
 *	DLNA_E_SUCCESS on success, nonzero on failure.
 ***************************************************************************/
static int
GetNameForAlias( IN char *name,
                 OUT char **alias )
{
    char *ext;
    char *al;

    ext = strrchr( name, '.' );
    if( ext == NULL || strcasecmp( ext, ".xml" ) != 0 ) {
        return DLNA_E_EXT_NOT_XML;
    }

    al = strrchr( name, '/' );
    if( al == NULL ) {
        *alias = name;
    } else {
        *alias = al;
    }

    return DLNA_E_SUCCESS;
}

/**************************************************************************
 * Function: get_server_addr
 *
 * Parameters:	
 *	OUT struct sockaddr_in* serverAddr: pointer to server address
 *		structure 
 *
 * Description:
 *	This function fills the sockadr_in with miniserver information.
 *
 * Return Values: VOID
 *      
 ***************************************************************************/
static void
get_server_addr( OUT struct sockaddr_in *serverAddr )
{
    memset( serverAddr, 0, sizeof( struct sockaddr_in ) );

    serverAddr->sin_family = AF_INET;
    serverAddr->sin_port = htons( LOCAL_PORT );
    //inet_aton( LOCAL_HOST, &serverAddr->sin_addr );
    serverAddr->sin_addr.s_addr = inet_addr( LOCAL_HOST );
}

/**************************************************************************
 * Function: GetDescDocumentAndURL ( In the case of device)
 *
 * Parameters:	
 *	IN dlna_DescType descriptionType: pointer to server address
 *		structure 
 *	IN char* description:
 *	IN unsigned int bufferLen:
 *	IN int config_baseURL:
 *	OUT IXML_Document **xmlDoc:
 *	OUT char descURL[LINE_SIZE]: 
 *
 * Description:
 *	This function fills the sockadr_in with miniserver information.
 *
 * Return Values: VOID
 *      
 ***************************************************************************/
static int
GetDescDocumentAndURL( IN dlna_DescType descriptionType,
                       IN char *description,
                       IN unsigned int bufferLen dlna_unused,
                       IN int config_baseURL,
                       OUT IXML_Document ** xmlDoc,
                       OUT char descURL[LINE_SIZE] )
{
    int retVal = 0;
    char *membuf = NULL;
    char aliasStr[LINE_SIZE];
    char *temp_str = NULL;
    FILE *fp = NULL;
    off_t fileLen;
    size_t num_read;
    time_t last_modified;
    struct stat file_info;
    struct sockaddr_in serverAddr;
    int rc = DLNA_E_SUCCESS;

    if( description == NULL ) {
        return DLNA_E_INVALID_PARAM;
    }
    // non-URL description must have configuration specified
    if( descriptionType != DLNAREG_URL_DESC && ( !config_baseURL ) ) {
        return DLNA_E_INVALID_PARAM;
    }
    // get XML doc and last modified time
    if( descriptionType == DLNAREG_URL_DESC ) {
        if( ( retVal =
              dlnaDownloadXmlDoc( description,
                                  xmlDoc ) ) != DLNA_E_SUCCESS ) {
            return retVal;
        }
        last_modified = time( NULL );
    } else if( descriptionType == DLNAREG_FILENAME_DESC ) {
        retVal = stat( description, &file_info );
        if( retVal == -1 ) {
            return DLNA_E_FILE_NOT_FOUND;
        }
        fileLen = file_info.st_size;
        last_modified = file_info.st_mtime;

        if( ( fp = fopen( description, "rb" ) ) == NULL ) {
            return DLNA_E_FILE_NOT_FOUND;
        }

        if( ( membuf = ( char * )malloc( fileLen + 1 ) ) == NULL ) {
            fclose( fp );
            return DLNA_E_OUTOF_MEMORY;
        }

        num_read = fread( membuf, 1, fileLen, fp );
        if( num_read != fileLen ) {
            fclose( fp );
            free( membuf );
            return DLNA_E_FILE_READ_ERROR;
        }

        membuf[fileLen] = 0;
        fclose( fp );
        rc = ixmlParseBufferEx( membuf, xmlDoc );
        free( membuf );
    } else if( descriptionType == DLNAREG_BUF_DESC ) {
        last_modified = time( NULL );
        rc = ixmlParseBufferEx( description, xmlDoc );
    } else {
        return DLNA_E_INVALID_PARAM;
    }

    if( rc != IXML_SUCCESS && descriptionType != DLNAREG_URL_DESC ) {
        if( rc == IXML_INSUFFICIENT_MEMORY ) {
            return DLNA_E_OUTOF_MEMORY;
        } else {
            return DLNA_E_INVALID_DESC;
        }
    }
    // determine alias
    if( config_baseURL ) {
        if( descriptionType == DLNAREG_BUF_DESC ) {
            strcpy( aliasStr, "description.xml" );
        } else                  // URL or filename
        {
            retVal = GetNameForAlias( description, &temp_str );
            if( retVal != DLNA_E_SUCCESS ) {
                ixmlDocument_free( *xmlDoc );
                return retVal;
            }
            if( strlen( temp_str ) > ( LINE_SIZE - 1 ) ) {
                ixmlDocument_free( *xmlDoc );
                free( temp_str );
                return DLNA_E_URL_TOO_BIG;
            }
            strcpy( aliasStr, temp_str );
        }

        get_server_addr( &serverAddr );

        // config
        retVal = configure_urlbase( *xmlDoc, &serverAddr,
                                    aliasStr, last_modified, descURL );
        if( retVal != DLNA_E_SUCCESS ) {
            ixmlDocument_free( *xmlDoc );
            return retVal;
        }
    } else                      // manual
    {
        if( strlen( description ) > ( LINE_SIZE - 1 ) ) {
            ixmlDocument_free( *xmlDoc );
            return DLNA_E_URL_TOO_BIG;
        }
        strcpy( descURL, description );
    }

    assert( *xmlDoc != NULL );

    return DLNA_E_SUCCESS;
}

#else // no web server

/**************************************************************************
 * Function: GetDescDocumentAndURL ( In the case of control point)
 *
 *  Parameters:	
 *	IN dlna_DescType descriptionType: pointer to server address
 *		structure 
 *	IN char* description:
 *	IN unsigned int bufferLen:
 *	IN int config_baseURL:
 *	OUT IXML_Document **xmlDoc:
 *	OUT char *descURL: 
 *
 * Description:
 *	This function fills the sockadr_in with miniserver information.
 *
 * Return Values: VOID
 *      
 ***************************************************************************/
static int
GetDescDocumentAndURL( IN dlna_DescType descriptionType,
                       IN char *description,
                       IN unsigned int bufferLen,
                       IN int config_baseURL,
                       OUT IXML_Document ** xmlDoc,
                       OUT char *descURL )
{
    int retVal;

    if( ( descriptionType != DLNAREG_URL_DESC ) || config_baseURL ) {
        return DLNA_E_NO_WEB_SERVER;
    }

    if( description == NULL ) {
        return DLNA_E_INVALID_PARAM;
    }

    if( strlen( description ) > ( LINE_SIZE - 1 ) ) {
        return DLNA_E_URL_TOO_BIG;
    }
    strcpy( descURL, description );

    if( ( retVal =
          dlnaDownloadXmlDoc( description, xmlDoc ) ) != DLNA_E_SUCCESS ) {
        return retVal;
    }

    return DLNA_E_SUCCESS;
}

#endif // INTERNAL_WEB_SERVER
// ********************************************************

/****************************************************************************
 * Function: dlnaRegisterRootDevice2
 *
 * Parameters:	
 *	IN dlna_DescType descriptionType: The type of description document.
 *	IN const char* description:  Treated as a URL, file name or 
 *		memory buffer depending on description type. 
 *	IN size_t bufferLen: Length of memory buffer if passing a description
 *		in a buffer, otherwize ignored.
 *	IN int config_baseURL: If nonzero, URLBase of description document is 
 *		configured and the description is served using the internal
 *		web server.
 *	IN dlna_FunPtr Fun: Pointer to the callback function for 
 *		receiving asynchronous events. 
 *	IN const void* Cookie: Pointer to user data returned with the 
 *		callback function when invoked. 
 *	OUT dlnaDevice_Handle* Hnd: Pointer to a variable to store 
 *		the new device handle.
 *
 * Description:
 *	This function is similar to  dlnaRegisterRootDevice except that
 *	it also allows the description document to be specified as a file or 
 *	a memory buffer. The description can also be configured to have the
 *	correct IP and port address.
 *
 * Return Values:
 *      DLNA_E_SUCCESS on success, nonzero on failure.
 *****************************************************************************/
int
dlnaRegisterRootDevice2( IN dlna_DescType descriptionType,
                         IN const char *description_const,
                         IN size_t bufferLen,   // ignored unless descType == DLNAREG_BUF_DESC

                         IN int config_baseURL,
                         IN dlna_FunPtr Fun,
                         IN const void *Cookie,
                         OUT dlnaDevice_Handle * Hnd )
{
    struct Handle_Info *HInfo;
    int retVal = 0;
    char *description = ( char * )description_const;
    if( dlnaSdkInit != 1 ) {
        return DLNA_E_FINISH;
    }

    dlnaPrintf( DLNA_INFO, API, __FILE__, __LINE__,
        "Inside dlnaRegisterRootDevice2\n" );

    if( Hnd == NULL || Fun == NULL ) {
        return DLNA_E_INVALID_PARAM;
    }

    HandleLock();
    if( dlnaSdkDeviceRegistered ) {
        HandleUnlock();
        return DLNA_E_ALREADY_REGISTERED;
    }

    if( ( *Hnd = GetFreeHandle() ) == DLNA_E_OUTOF_HANDLE ) {
        HandleUnlock();
        return DLNA_E_OUTOF_MEMORY;
    }

    HInfo = ( struct Handle_Info * )malloc( sizeof( struct Handle_Info ) );
    if( HInfo == NULL ) {
        HandleUnlock();
        return DLNA_E_OUTOF_MEMORY;
    }
    HandleTable[*Hnd] = HInfo;

    // prevent accidental removal of a non-existent alias
    HInfo->aliasInstalled = 0;

    retVal = GetDescDocumentAndURL(
        descriptionType, description, bufferLen,
        config_baseURL, &HInfo->DescDocument, HInfo->DescURL );

    if( retVal != DLNA_E_SUCCESS ) {
        FreeHandle( *Hnd );
        HandleUnlock();
        return retVal;
    }

    HInfo->aliasInstalled = ( config_baseURL != 0 );
    HInfo->HType = HND_DEVICE;

    HInfo->Callback = Fun;
    HInfo->Cookie = ( void * )Cookie;
    HInfo->MaxAge = DEFAULT_MAXAGE;
    HInfo->DeviceList = NULL;
    HInfo->ServiceList = NULL;

    CLIENTONLY( ListInit( &HInfo->SsdpSearchList, NULL, NULL ); )
    CLIENTONLY( HInfo->ClientSubList = NULL; )
    HInfo->MaxSubscriptions = DLNA_INFINITE;
    HInfo->MaxSubscriptionTimeOut = DLNA_INFINITE;

    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "dlnaRegisterRootDevice2: Valid Description\n" );
    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "dlnaRegisterRootDevice2: DescURL : %s\n",
        HInfo->DescURL );

    HInfo->DeviceList =
        ixmlDocument_getElementsByTagName( HInfo->DescDocument, "device" );

    if( !HInfo->DeviceList ) {
        CLIENTONLY( ListDestroy( &HInfo->SsdpSearchList, 0 ); )
        ixmlDocument_free( HInfo->DescDocument );
        FreeHandle( *Hnd );
        HandleUnlock();
        dlnaPrintf( DLNA_INFO, API, __FILE__, __LINE__,
            "dlnaRegisterRootDevice2: No devices found for RootDevice\n" );
        return DLNA_E_INVALID_DESC;
    }

    HInfo->ServiceList = ixmlDocument_getElementsByTagName(
        HInfo->DescDocument, "serviceList" );
    if( !HInfo->ServiceList ) {
        dlnaPrintf( DLNA_INFO, API, __FILE__, __LINE__,
            "dlnaRegisterRootDevice2: No services found for RootDevice\n" );
    }

    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "dlnaRegisterRootDevice2: Gena Check\n" );
    //*******************************
    // GENA SET UP
    //*******************************
    if( getServiceTable( ( IXML_Node * ) HInfo->DescDocument,
            &HInfo->ServiceTable, HInfo->DescURL ) ) {
        dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
            "dlnaRegisterRootDevice2: GENA Service Table\n" );
        dlnaPrintf( DLNA_INFO, API, __FILE__, __LINE__,
            "Here are the known services: \n" );
        printServiceTable( &HInfo->ServiceTable, DLNA_INFO, API );
    } else {
        dlnaPrintf( DLNA_INFO, API, __FILE__, __LINE__,
            "\ndlnaRegisterRootDevice2: Empty service table\n" );
    }

    dlnaSdkDeviceRegistered = 1;
    HandleUnlock();
    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "Exiting RegisterRootDevice2 Successfully\n" );

    return DLNA_E_SUCCESS;
}

#endif // INCLUDE_DEVICE_APIS

#ifdef INCLUDE_CLIENT_APIS

/**************************************************************************
 * Function: dlnaRegisterClient
 *
 * Parameters:	
 *	IN dlna_FunPtr Fun:  Pointer to a function for receiving 
 *		 asynchronous events.
 *	IN const void * Cookie: Pointer to user data returned with the 
 *		callback function when invoked.
 *	OUT dlnaClient_Handle *Hnd: Pointer to a variable to store 
 *		the new control point handle.
 *
 * Description:
 *	This function registers a control point application with the
 *	UPnP Library.  A control point application cannot make any other API 
 *	calls until it registers using this function.
 *
 * Return Values: int
 *      
 ***************************************************************************/
int
dlnaRegisterClient( IN dlna_FunPtr Fun,
                    IN const void *Cookie,
                    OUT dlnaClient_Handle * Hnd )
{
    struct Handle_Info *HInfo;

    if( dlnaSdkInit != 1 ) {
        return DLNA_E_FINISH;
    }
    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "Inside dlnaRegisterClient \n" );
    if( Fun == NULL || Hnd == NULL ) {
        return DLNA_E_INVALID_PARAM;
    }

    HandleLock();

    if( dlnaSdkClientRegistered ) {
        HandleUnlock();
        return DLNA_E_ALREADY_REGISTERED;
    }
    if( ( *Hnd = GetFreeHandle() ) == DLNA_E_OUTOF_HANDLE ) {
        HandleUnlock();
        return DLNA_E_OUTOF_MEMORY;
    }
    HInfo = ( struct Handle_Info * )malloc( sizeof( struct Handle_Info ) );
    if( HInfo == NULL ) {
        HandleUnlock();
        return DLNA_E_OUTOF_MEMORY;
    }

    HInfo->HType = HND_CLIENT;
    HInfo->Callback = Fun;
    HInfo->Cookie = ( void * )Cookie;
    HInfo->ClientSubList = NULL;
    ListInit( &HInfo->SsdpSearchList, NULL, NULL );
#ifdef INCLUDE_DEVICE_APIS
    HInfo->MaxAge = 0;
    HInfo->MaxSubscriptions = DLNA_INFINITE;
    HInfo->MaxSubscriptionTimeOut = DLNA_INFINITE;
#endif

    HandleTable[*Hnd] = HInfo;
    dlnaSdkClientRegistered = 1;

    HandleUnlock();

    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "Exiting dlnaRegisterClient \n" );

    return DLNA_E_SUCCESS;

}  /****************** End of dlnaRegisterClient   *********************/
#endif // INCLUDE_CLIENT_APIS


/****************************************************************************
 * Function: dlnaUnRegisterClient
 *
 * Parameters:	
 *	IN dlnaClient_Handle Hnd: The handle of the control point instance 
 *		to unregister
 * Description:
 *	This function unregisters a client registered with 
 *	dlnaRegisterclient or dlnaRegisterclient2. After this call, the 
 *	dlnaDevice_Handle Hnd is no longer valid. The UPnP Library generates 
 *	no more callbacks after this function returns.
 *
 * Return Values:
 *	DLNA_E_SUCCESS on success, nonzero on failure.
 *****************************************************************************/
#ifdef INCLUDE_CLIENT_APIS
int
dlnaUnRegisterClient( IN dlnaClient_Handle Hnd )
{
    struct Handle_Info *HInfo;
    ListNode *node = NULL;
    SsdpSearchArg *searchArg = NULL;

    if( dlnaSdkInit != 1 ) {
        return DLNA_E_FINISH;
    }

    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "Inside dlnaUnRegisterClient \n" );
    HandleLock();
    if( !dlnaSdkClientRegistered ) {
        HandleUnlock();
        return DLNA_E_INVALID_HANDLE;
    }
    HandleUnlock();

#if EXCLUDE_GENA == 0
    if( genaUnregisterClient( Hnd ) != DLNA_E_SUCCESS )
        return DLNA_E_INVALID_HANDLE;
#endif
    HandleLock();
    if( GetHandleInfo( Hnd, &HInfo ) == DLNA_E_INVALID_HANDLE ) {
        HandleUnlock();
        return DLNA_E_INVALID_HANDLE;
    }
    //clean up search list
    node = ListHead( &HInfo->SsdpSearchList );
    while( node != NULL ) {
        searchArg = ( SsdpSearchArg * ) node->item;
        if( searchArg ) {
            free( searchArg->searchTarget );
            free( searchArg );
        }
        ListDelNode( &HInfo->SsdpSearchList, node, 0 );
        node = ListHead( &HInfo->SsdpSearchList );
    }

    ListDestroy( &HInfo->SsdpSearchList, 0 );
    FreeHandle( Hnd );
    dlnaSdkClientRegistered = 0;
    HandleUnlock();
    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "Exiting dlnaUnRegisterClient \n" );
    return DLNA_E_SUCCESS;

}  /****************** End of dlnaUnRegisterClient *********************/
#endif // INCLUDE_CLIENT_APIS

//-----------------------------------------------------------------------------
//
//                                   SSDP interface
//
//-----------------------------------------------------------------------------

#ifdef INCLUDE_DEVICE_APIS
#if EXCLUDE_SSDP == 0

/**************************************************************************
 * Function: dlnaSendAdvertisement 
 *
 * Parameters:	
 *	IN dlnaDevice_Handle Hnd: handle of the device instance
 *	IN int Exp : Timer for resending the advertisement
 *
 * Description:
 *	This function sends the device advertisement. It also schedules a
 *	job for the next advertisement after "Exp" time.
 *
 * Return Values: int
 *	DLNA_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int
dlnaSendAdvertisement( IN dlnaDevice_Handle Hnd,
                       IN int Exp )
{
    struct Handle_Info *SInfo = NULL;
    int retVal = 0,
     *ptrMx;
    dlna_timeout *adEvent;
    ThreadPoolJob job;

    if( dlnaSdkInit != 1 ) {
        return DLNA_E_FINISH;
    }

    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "Inside dlnaSendAdvertisement \n" );

    HandleLock();
    if( GetHandleInfo( Hnd, &SInfo ) != HND_DEVICE ) {
        HandleUnlock();
        return DLNA_E_INVALID_HANDLE;
    }
    if( Exp < 1 )
        Exp = DEFAULT_MAXAGE;
    SInfo->MaxAge = Exp;
    HandleUnlock();
    retVal = AdvertiseAndReply( 1, Hnd, 0, ( struct sockaddr_in * )NULL,
                                ( char * )NULL, ( char * )NULL,
                                ( char * )NULL, Exp );

    if( retVal != DLNA_E_SUCCESS )
        return retVal;
    ptrMx = ( int * )malloc( sizeof( int ) );
    if( ptrMx == NULL )
        return DLNA_E_OUTOF_MEMORY;
    adEvent = ( dlna_timeout * ) malloc( sizeof( dlna_timeout ) );

    if( adEvent == NULL ) {
        free( ptrMx );
        return DLNA_E_OUTOF_MEMORY;
    }
    *ptrMx = Exp;
    adEvent->handle = Hnd;
    adEvent->Event = ptrMx;

    HandleLock();
    if( GetHandleInfo( Hnd, &SInfo ) != HND_DEVICE ) {
        HandleUnlock();
        free( adEvent );
        free( ptrMx );
        return DLNA_E_INVALID_HANDLE;
    }
#ifdef SSDP_PACKET_DISTRIBUTE
    TPJobInit( &job, ( start_routine ) AutoAdvertise, adEvent );
    TPJobSetFreeFunction( &job, ( free_routine ) free_dlna_timeout );
    TPJobSetPriority( &job, MED_PRIORITY );
    if( ( retVal = TimerThreadSchedule( &gTimerThread,
                                        ( ( Exp / 2 ) -
                                          ( AUTO_ADVERTISEMENT_TIME ) ),
                                        REL_SEC, &job, SHORT_TERM,
                                        &( adEvent->eventId ) ) )
        != DLNA_E_SUCCESS ) {
        HandleUnlock();
        free( adEvent );
        free( ptrMx );
        return retVal;
    }
#else
    TPJobInit( &job, ( start_routine ) AutoAdvertise, adEvent );
    TPJobSetFreeFunction( &job, ( free_routine ) free_dlna_timeout );
    TPJobSetPriority( &job, MED_PRIORITY );
    if( ( retVal = TimerThreadSchedule( &gTimerThread,
                                        Exp - AUTO_ADVERTISEMENT_TIME,
                                        REL_SEC, &job, SHORT_TERM,
                                        &( adEvent->eventId ) ) )
        != DLNA_E_SUCCESS ) {
        HandleUnlock();
        free( adEvent );
        free( ptrMx );
        return retVal;
    }
#endif

    HandleUnlock();
    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "Exiting dlnaSendAdvertisement \n" );

    return retVal;

}  /****************** End of dlnaSendAdvertisement *********************/
#endif // INCLUDE_DEVICE_APIS
#endif
#if EXCLUDE_SSDP == 0
#ifdef INCLUDE_CLIENT_APIS

/**************************************************************************
 * Function: dlnaSearchAsync 
 *
 * Parameters:	
 *	IN dlnaClient_Handle Hnd: handle of the control point instance
 *	IN int Mx : Maximum time to wait for the search reply
 *	IN const char *Target_const: 
 *	IN const void *Cookie_const:
 *
 * Description:
 *	This function searches for the devices for the provided maximum time.
 *	It is a asynchronous function. It schedules a search job and returns. 
 *	client is notified about the search results after search timer.
 *
 * Return Values: int
 *	DLNA_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int
dlnaSearchAsync( IN dlnaClient_Handle Hnd,
                 IN int Mx,
                 IN const char *Target_const,
                 IN const void *Cookie_const )
{
    struct Handle_Info *SInfo = NULL;
    char *Target = ( char * )Target_const;

    if( dlnaSdkInit != 1 ) {
        return DLNA_E_FINISH;
    }

    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "Inside dlnaSearchAsync \n" );

    HandleReadLock();
    if( GetHandleInfo( Hnd, &SInfo ) != HND_CLIENT ) {
        HandleUnlock();
        return DLNA_E_INVALID_HANDLE;
    }
    if( Mx < 1 )
        Mx = DEFAULT_MX;

    if( Target == NULL ) {
        HandleUnlock();
        return DLNA_E_INVALID_PARAM;
    }

    HandleUnlock();
    SearchByTarget( Mx, Target, ( void * )Cookie_const );

    //HandleUnlock();

    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "Exiting dlnaSearchAsync \n" );

    return DLNA_E_SUCCESS;

}  /****************** End of dlnaSearchAsync *********************/
#endif // INCLUDE_CLIENT_APIS
#endif
//-----------------------------------------------------------------------------
//
//                                   GENA interface 
//
//-----------------------------------------------------------------------------

#if EXCLUDE_GENA == 0
#ifdef INCLUDE_DEVICE_APIS

/**************************************************************************
 * Function: dlnaSetMaxSubscriptions 
 *
 * Parameters:	
 *	IN dlnaDevice_Handle Hnd: The handle of the device for which
 *		the maximum subscriptions is being set.
 *	IN int MaxSubscriptions: The maximum number of subscriptions to be
 *		allowed per service.
 *
 * Description:
 *	This function sets the maximum subscriptions of the control points
 * Return Values: int
 *	DLNA_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int
dlnaSetMaxSubscriptions( IN dlnaDevice_Handle Hnd,
                         IN int MaxSubscriptions )
{
    struct Handle_Info *SInfo = NULL;

    if( dlnaSdkInit != 1 ) {
        return DLNA_E_FINISH;
    }

    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "Inside dlnaSetMaxSubscriptions \n" );

    HandleLock();
    if( ( ( MaxSubscriptions != DLNA_INFINITE )
          && ( MaxSubscriptions < 0 ) )
        || ( GetHandleInfo( Hnd, &SInfo ) != HND_DEVICE ) ) {
        HandleUnlock();
        return DLNA_E_INVALID_HANDLE;
    }
    SInfo->MaxSubscriptions = MaxSubscriptions;
    HandleUnlock();

    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "Exiting dlnaSetMaxSubscriptions \n" );

    return DLNA_E_SUCCESS;

}  /***************** End of dlnaSetMaxSubscriptions ********************/
#endif // INCLUDE_DEVICE_APIS

#ifdef INCLUDE_DEVICE_APIS

/**************************************************************************
 * Function: dlnaSetMaxSubscriptionTimeOut 
 *
 * Parameters:	
 *	IN dlnaDevice_Handle Hnd: The handle of the device for which the
 *		maximum subscription time-out is being set.
 *	IN int MaxSubscriptionTimeOut:The maximum subscription time-out 
 *		to be accepted
 *
 * Description:
 *	This function sets the maximum subscription timer. Control points
 *	will require to send the subscription request before timeout.
 *
 * Return Values: int
 *	DLNA_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int
dlnaSetMaxSubscriptionTimeOut( IN dlnaDevice_Handle Hnd,
                               IN int MaxSubscriptionTimeOut )
{
    struct Handle_Info *SInfo = NULL;

    if( dlnaSdkInit != 1 ) {
        return DLNA_E_FINISH;
    }

    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "Inside dlnaSetMaxSubscriptionTimeOut \n" );

    HandleLock();

    if( ( ( MaxSubscriptionTimeOut != DLNA_INFINITE )
          && ( MaxSubscriptionTimeOut < 0 ) )
        || ( GetHandleInfo( Hnd, &SInfo ) != HND_DEVICE ) ) {
        HandleUnlock();
        return DLNA_E_INVALID_HANDLE;
    }

    SInfo->MaxSubscriptionTimeOut = MaxSubscriptionTimeOut;
    HandleUnlock();

    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "Exiting dlnaSetMaxSubscriptionTimeOut \n" );

    return DLNA_E_SUCCESS;

}  /****************** End of dlnaSetMaxSubscriptionTimeOut ******************/
#endif // INCLUDE_DEVICE_APIS

#ifdef INCLUDE_CLIENT_APIS

/**************************************************************************
 * Function: dlnaSubscribeAsync 
 *
 * Parameters:	
 *	IN dlnaClient_Handle Hnd: The handle of the control point for which 
 *		the subscription request is to be sent.
 *	IN const char * EvtUrl_const: URL that control point wants to 
 *		subscribe
 *	IN int TimeOut: The requested subscription time.  Upon 
 *		return, it contains the actual subscription time 
 *		returned from the service
 *	IN dlna_FunPtr Fun : callback function to tell result of the 
 *		subscription request
 *	IN const void * Cookie_const: cookie passed by client to give back 
 *		in the callback function.
 *
 * Description:
 *	This function performs the same operation as dlnaSubscribeAsync
 *	but returns immediately and calls the registered callback function 
 *	when the operation is complete.
 *
 * Return Values: int
 *	DLNA_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int
dlnaSubscribeAsync( IN dlnaClient_Handle Hnd,
                    IN const char *EvtUrl_const,
                    IN int TimeOut,
                    IN dlna_FunPtr Fun,
                    IN const void *Cookie_const )
{
    struct Handle_Info *SInfo = NULL;
    struct dlnaNonblockParam *Param;
    char *EvtUrl = ( char * )EvtUrl_const;
    ThreadPoolJob job;

    if( dlnaSdkInit != 1 ) {
        return DLNA_E_FINISH;
    }

    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "Inside dlnaSubscribeAsync \n" );

    HandleReadLock();
    if( GetHandleInfo( Hnd, &SInfo ) != HND_CLIENT ) {
        HandleUnlock();
        return DLNA_E_INVALID_HANDLE;
    }
    if( EvtUrl == NULL ) {
        HandleUnlock();
        return DLNA_E_INVALID_PARAM;
    }
    if( TimeOut != DLNA_INFINITE && TimeOut < 1 ) {
        HandleUnlock();
        return DLNA_E_INVALID_PARAM;
    }
    if( Fun == NULL ) {
        HandleUnlock();
        return DLNA_E_INVALID_PARAM;
    }
    HandleUnlock();

    Param = (struct dlnaNonblockParam *)
        malloc(sizeof (struct dlnaNonblockParam));
    if( Param == NULL ) {
        return DLNA_E_OUTOF_MEMORY;
    }

    Param->FunName = SUBSCRIBE;
    Param->Handle = Hnd;
    strcpy( Param->Url, EvtUrl );
    Param->TimeOut = TimeOut;
    Param->Fun = Fun;
    Param->Cookie = ( void * )Cookie_const;

    TPJobInit( &job, ( start_routine ) dlnaThreadDistribution, Param );
    TPJobSetFreeFunction( &job, ( free_routine ) free );
    TPJobSetPriority( &job, MED_PRIORITY );
    ThreadPoolAdd( &gSendThreadPool, &job, NULL );

    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "Exiting dlnaSubscribeAsync \n" );

    return DLNA_E_SUCCESS;

}  /****************** End of dlnaSubscribeAsync *********************/
#endif // INCLUDE_CLIENT_APIS

#ifdef INCLUDE_CLIENT_APIS

/**************************************************************************
 * Function: dlnaSubscribe 
 *
 * Parameters:	
 *	IN dlnaClient_Handle Hnd: The handle of the control point.
 *	IN const char *PublisherUrl: The URL of the service to subscribe to.
 *	INOUT int *TimeOut: Pointer to a variable containing the requested 
 *		subscription time.  Upon return, it contains the
 *		actual subscription time returned from the service.
 *	OUT dlna_SID SubsId: Pointer to a variable to receive the 
 *		subscription ID (SID). 
 *
 * Description:
 *	This function registers a control point to receive event
 *	notifications from another device.  This operation is synchronous
 *
 * Return Values: int
 *	DLNA_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int
dlnaSubscribe( IN dlnaClient_Handle Hnd,
               IN const char *EvtUrl_const,
               INOUT int *TimeOut,
               OUT dlna_SID SubsId )
{
    struct Handle_Info *SInfo = NULL;
    int RetVal;
    char *EvtUrl = ( char * )EvtUrl_const;

    if( dlnaSdkInit != 1 ) {
        return DLNA_E_FINISH;
    }

    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "Inside dlnaSubscribe \n" );

    HandleReadLock();
    if( GetHandleInfo( Hnd, &SInfo ) != HND_CLIENT ) {
        HandleUnlock();
        return DLNA_E_INVALID_HANDLE;
    }
    if( EvtUrl == NULL ) {
        HandleUnlock();
        return DLNA_E_INVALID_PARAM;
    }
    if( TimeOut == NULL ) {
        HandleUnlock();
        return DLNA_E_INVALID_PARAM;
    }
    if( SubsId == NULL ) {
        HandleUnlock();
        return DLNA_E_INVALID_PARAM;
    }
    HandleUnlock();
    RetVal = genaSubscribe( Hnd, EvtUrl, TimeOut, SubsId );

    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "Exiting dlnaSubscribe \n" );

    return RetVal;

}  /****************** End of dlnaSubscribe  *********************/
#endif // INCLUDE_CLIENT_APIS

#ifdef INCLUDE_CLIENT_APIS

/**************************************************************************
 * Function: dlnaUnSubscribe 
 *
 *  Parameters:	
 *	IN dlnaClient_Handle Hnd: The handle of the control point.
 *	IN dlna_SID SubsId: The ID returned when the control point 
 *		subscribed to the service.
 *
 * Description:
 *	This function removes the subscription of  a control point from a 
 *	service previously subscribed to using dlnaSubscribe or 
 *	dlnaSubscribeAsync. This is a synchronous call.
 *
 * Return Values: int
 *	DLNA_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int
dlnaUnSubscribe( IN dlnaClient_Handle Hnd,
                 IN dlna_SID SubsId )
{
    struct Handle_Info *SInfo = NULL;
    int RetVal;

    if( dlnaSdkInit != 1 ) {
        return DLNA_E_FINISH;
    }

    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "Inside dlnaUnSubscribe \n" );

    HandleReadLock();
    if( GetHandleInfo( Hnd, &SInfo ) != HND_CLIENT ) {
        HandleUnlock();
        return DLNA_E_INVALID_HANDLE;
    }
    if( SubsId == NULL ) {
        HandleUnlock();
        return DLNA_E_INVALID_PARAM;
    }
    HandleUnlock();
    RetVal = genaUnSubscribe( Hnd, SubsId );

    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "Exiting dlnaUnSubscribe \n" );

    return RetVal;

}  /****************** End of dlnaUnSubscribe  *********************/
#endif // INCLUDE_CLIENT_APIS

#ifdef INCLUDE_CLIENT_APIS

/**************************************************************************
 * Function: dlnaUnSubscribeAsync 
 *
 *  Parameters:	
 *	IN dlnaClient_Handle Hnd: The handle of the subscribed control point. 
 *	IN dlna_SID SubsId: The ID returned when the control point 
 *		subscribed to the service.
 *	IN dlna_FunPtr Fun: Pointer to a callback function to be called
 *		when the operation is complete. 
 *	IN const void *Cookie:Pointer to user data to pass to the
 *		callback function when invoked.
 *
 *  Description:
 *      This function removes a subscription of a control point
 *  from a service previously subscribed to using dlnaSubscribe or
 *	dlnaSubscribeAsync,generating a callback when the operation is complete.
 *
 *  Return Values: int
 *      DLNA_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int
dlnaUnSubscribeAsync( IN dlnaClient_Handle Hnd,
                      IN dlna_SID SubsId,
                      IN dlna_FunPtr Fun,
                      IN const void *Cookie_const )
{
    ThreadPoolJob job;
    struct Handle_Info *SInfo = NULL;
    struct dlnaNonblockParam *Param;

    if( dlnaSdkInit != 1 ) {
        return DLNA_E_FINISH;
    }

    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "Inside dlnaUnSubscribeAsync \n" );

    HandleReadLock();
    if( GetHandleInfo( Hnd, &SInfo ) != HND_CLIENT ) {
        HandleUnlock();
        return DLNA_E_INVALID_HANDLE;
    }
    if( SubsId == NULL ) {
        HandleUnlock();
        return DLNA_E_INVALID_PARAM;
    }
    if( Fun == NULL ) {
        HandleUnlock();
        return DLNA_E_INVALID_PARAM;
    }

    HandleUnlock();
    Param =
        ( struct dlnaNonblockParam * )
        malloc( sizeof( struct dlnaNonblockParam ) );
    if( Param == NULL )
        return DLNA_E_OUTOF_MEMORY;

    Param->FunName = UNSUBSCRIBE;
    Param->Handle = Hnd;
    strcpy( Param->SubsId, SubsId );
    Param->Fun = Fun;
    Param->Cookie = ( void * )Cookie_const;
    TPJobInit( &job, ( start_routine ) dlnaThreadDistribution, Param );
    TPJobSetFreeFunction( &job, ( free_routine ) free );
    TPJobSetPriority( &job, MED_PRIORITY );
    ThreadPoolAdd( &gSendThreadPool, &job, NULL );

    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "Exiting dlnaUnSubscribeAsync \n" );

    return DLNA_E_SUCCESS;

}  /****************** End of dlnaUnSubscribeAsync  *********************/
#endif // INCLUDE_CLIENT_APIS

#ifdef INCLUDE_CLIENT_APIS

/**************************************************************************
 * Function: dlnaRenewSubscription 
 *
 * Parameters:	
 *	IN dlnaClient_Handle Hnd: The handle of the control point that 
 *		is renewing the subscription.
 *	INOUT int *TimeOut: Pointer to a variable containing the 
 *		requested subscription time.  Upon return, 
 *		it contains the actual renewal time. 
 *	IN dlna_SID SubsId: The ID for the subscription to renew. 
 *
 * Description:
 *	This function renews a subscription that is about to 
 *	expire.  This function is synchronous.
 *
 * Return Values: int
 *	DLNA_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int
dlnaRenewSubscription( IN dlnaClient_Handle Hnd,
                       INOUT int *TimeOut,
                       IN dlna_SID SubsId )
{
    struct Handle_Info *SInfo = NULL;
    int RetVal;

    if( dlnaSdkInit != 1 ) {
        return DLNA_E_FINISH;
    }

    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "Inside dlnaRenewSubscription \n" );

    HandleReadLock();
    if( GetHandleInfo( Hnd, &SInfo ) != HND_CLIENT ) {
        HandleUnlock();
        return DLNA_E_INVALID_HANDLE;
    }
    if( TimeOut == NULL ) {
        HandleUnlock();
        return DLNA_E_INVALID_PARAM;
    }
    if( SubsId == NULL ) {
        HandleUnlock();
        return DLNA_E_INVALID_PARAM;
    }
    HandleUnlock();
    RetVal = genaRenewSubscription( Hnd, SubsId, TimeOut );

    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "Exiting dlnaRenewSubscription \n" );

    return RetVal;

}  /****************** End of dlnaRenewSubscription  *********************/
#endif // INCLUDE_CLIENT_APIS

#ifdef INCLUDE_CLIENT_APIS

/**************************************************************************
 * Function: dlnaRenewSubscriptionAsync 
 *
 * Parameters:	
 *	IN dlnaClient_Handle Hnd: The handle of the control point that 
 *		is renewing the subscription. 
 *	IN int TimeOut: The requested subscription time.  The 
 *		actual timeout value is returned when 
 *		the callback function is called. 
 *	IN dlna_SID SubsId: The ID for the subscription to renew. 
 *	IN dlna_FunPtr Fun: Pointer to a callback function to be 
 *		invoked when the renewal is complete. 
 *	IN const void *Cookie  : Pointer to user data passed 
 *		to the callback function when invoked.
 *
 * Description:
 *	This function renews a subscription that is about
 *	to expire, generating a callback when the operation is complete.
 *
 * Return Values: int
 *	DLNA_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int
dlnaRenewSubscriptionAsync( IN dlnaClient_Handle Hnd,
                            INOUT int TimeOut,
                            IN dlna_SID SubsId,
                            IN dlna_FunPtr Fun,
                            IN const void *Cookie_const )
{
    ThreadPoolJob job;
    struct Handle_Info *SInfo = NULL;
    struct dlnaNonblockParam *Param;

    if( dlnaSdkInit != 1 ) {
        return DLNA_E_FINISH;
    }

    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "Inside dlnaRenewSubscriptionAsync \n" );
    HandleReadLock();
    if( GetHandleInfo( Hnd, &SInfo ) != HND_CLIENT ) {
        HandleUnlock();
        return DLNA_E_INVALID_HANDLE;
    }
    if( TimeOut != DLNA_INFINITE && TimeOut < 1 ) {
        HandleUnlock();
        return DLNA_E_INVALID_PARAM;
    }
    if( SubsId == NULL ) {
        HandleUnlock();
        return DLNA_E_INVALID_PARAM;
    }
    if( Fun == NULL ) {
        HandleUnlock();
        return DLNA_E_INVALID_PARAM;
    }
    HandleUnlock();

    Param =
        ( struct dlnaNonblockParam * )
        malloc( sizeof( struct dlnaNonblockParam ) );
    if( Param == NULL ) {
        return DLNA_E_OUTOF_MEMORY;
    }

    Param->FunName = RENEW;
    Param->Handle = Hnd;
    strcpy( Param->SubsId, SubsId );
    Param->Fun = Fun;
    Param->Cookie = ( void * )Cookie_const;
    Param->TimeOut = TimeOut;

    TPJobInit( &job, ( start_routine ) dlnaThreadDistribution, Param );
    TPJobSetFreeFunction( &job, ( free_routine ) free );
    TPJobSetPriority( &job, MED_PRIORITY );
    ThreadPoolAdd( &gSendThreadPool, &job, NULL );

    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "Exiting dlnaRenewSubscriptionAsync \n" );

    return DLNA_E_SUCCESS;

}  /****************** End of dlnaRenewSubscriptionAsync *******************/
#endif // INCLUDE_CLIENT_APIS

#ifdef INCLUDE_DEVICE_APIS

/**************************************************************************
 * Function: dlnaNotify 
 *
 *  Parameters:	
 *	IN dlnaDevice_Handle: The handle to the device sending the event.
 *	IN const char *DevID: The device ID of the subdevice of the 
 *		service generating the event. 
 *	IN const char *ServID: The unique identifier of the service 
 *		generating the event. 
 *	IN const char **VarName: Pointer to an array of variables that 
 *		have changed.
 *	IN const char **NewVal: Pointer to an array of new values for 
 *		those variables. 
 *	IN int cVariables: The count of variables included in this 
 *		notification. 
 *
 * Description:
 *	This function sends out an event change notification to all
 *	control points subscribed to a particular service.  This function is
 *	synchronous and generates no callbacks.
 *
 * Return Values: int
 *	DLNA_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int
dlnaNotify( IN dlnaDevice_Handle Hnd,
            IN const char *DevID_const,
            IN const char *ServName_const,
            IN const char **VarName_const,
            IN const char **NewVal_const,
            IN int cVariables )
{

    struct Handle_Info *SInfo = NULL;
    int retVal;
    char *DevID = ( char * )DevID_const;
    char *ServName = ( char * )ServName_const;
    char **VarName = ( char ** )VarName_const;
    char **NewVal = ( char ** )NewVal_const;

    if( dlnaSdkInit != 1 ) {
        return DLNA_E_FINISH;
    }

    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "Inside dlnaNotify \n" );

    HandleReadLock();
    if( GetHandleInfo( Hnd, &SInfo ) != HND_DEVICE ) {
        HandleUnlock();
        return DLNA_E_INVALID_HANDLE;
    }
    if( DevID == NULL ) {
        HandleUnlock();
        return DLNA_E_INVALID_PARAM;
    }
    if( ServName == NULL ) {
        HandleUnlock();
        return DLNA_E_INVALID_PARAM;
    }
    if( VarName == NULL || NewVal == NULL || cVariables < 0 ) {
        HandleUnlock();
        return DLNA_E_INVALID_PARAM;
    }

    HandleUnlock();
    retVal =
        genaNotifyAll( Hnd, DevID, ServName, VarName, NewVal, cVariables );

    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "Exiting dlnaNotify \n" );

    return retVal;

} /****************** End of dlnaNotify *********************/

/**************************************************************************
 * Function: dlnaNotifyExt 
 *
 * Parameters:	
 *	IN dlnaDevice_Handle: The handle to the device sending the 
 *		event.
 *	IN const char *DevID: The device ID of the subdevice of the 
 *		service generating the event.
 *	IN const char *ServID: The unique identifier of the service 
 *		generating the event. 
 *	IN IXML_Document *PropSet: The DOM document for the property set. 
 *		Property set documents must conform to the XML schema
 *		defined in section 4.3 of the Universal Plug and Play
 *		Device Architecture specification. 
 *
 * Description:
 *	This function is similar to dlnaNotify except that it takes
 *	a DOM document for the event rather than an array of strings. This 
 *	function is synchronous and generates no callbacks.
 *
 * Return Values: int
 *	DLNA_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int
dlnaNotifyExt( IN dlnaDevice_Handle Hnd,
               IN const char *DevID_const,
               IN const char *ServName_const,
               IN IXML_Document * PropSet )
{

    struct Handle_Info *SInfo = NULL;
    int retVal;
    char *DevID = ( char * )DevID_const;
    char *ServName = ( char * )ServName_const;

    if( dlnaSdkInit != 1 ) {
        return DLNA_E_FINISH;
    }

    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "Inside dlnaNotify \n" );

    HandleReadLock();
    if( GetHandleInfo( Hnd, &SInfo ) != HND_DEVICE ) {
        HandleUnlock();
        return DLNA_E_INVALID_HANDLE;
    }
    if( DevID == NULL ) {
        HandleUnlock();
        return DLNA_E_INVALID_PARAM;
    }
    if( ServName == NULL ) {
        HandleUnlock();
        return DLNA_E_INVALID_PARAM;
    }

    HandleUnlock();
    retVal = genaNotifyAllExt( Hnd, DevID, ServName, PropSet );

    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "Exiting dlnaNotify \n" );

    return retVal;

}  /****************** End of dlnaNotify *********************/

#endif // INCLUDE_DEVICE_APIS

#ifdef INCLUDE_DEVICE_APIS

/**************************************************************************
 * Function: dlnaAcceptSubscription 
 *
 * Parameters:	
 *	IN dlnaDevice_Handle Hnd: The handle of the device. 
 *	IN const char *DevID: The device ID of the subdevice of the 
 *		service generating the event. 
 *	IN const char *ServID: The unique service identifier of the 
 *		service generating the event.
 *	IN const char **VarName: Pointer to an array of event variables.
 *	IN const char **NewVal: Pointer to an array of values for 
 *		the event variables.
 *	IN int cVariables: The number of event variables in VarName. 
 *	IN dlna_SID SubsId: The subscription ID of the newly 
 *		registered control point. 
 *
 * Description:
 *	This function accepts a subscription request and sends
 *	out the current state of the eventable variables for a service.  
 *	The device application should call this function when it receives a 
 *	DLNA_EVENT_SUBSCRIPTION_REQUEST callback. This function is sychronous
 *	and generates no callbacks.
 *
 * Return Values: int
 *	DLNA_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int
dlnaAcceptSubscription( IN dlnaDevice_Handle Hnd,
                        IN const char *DevID_const,
                        IN const char *ServName_const,
                        IN const char **VarName_const,
                        IN const char **NewVal_const,
                        int cVariables,
                        IN dlna_SID SubsId )
{
    struct Handle_Info *SInfo = NULL;
    int retVal;
    char *DevID = ( char * )DevID_const;
    char *ServName = ( char * )ServName_const;
    char **VarName = ( char ** )VarName_const;
    char **NewVal = ( char ** )NewVal_const;

    if( dlnaSdkInit != 1 ) {
        return DLNA_E_FINISH;
    }

    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "Inside dlnaAcceptSubscription \n" );

    HandleReadLock();
    if( GetHandleInfo( Hnd, &SInfo ) != HND_DEVICE ) {
        HandleUnlock();
        return DLNA_E_INVALID_HANDLE;
    }
    if( DevID == NULL ) {
        HandleUnlock();
        return DLNA_E_INVALID_PARAM;
    }
    if( ServName == NULL ) {
        HandleUnlock();
        return DLNA_E_INVALID_PARAM;
    }
    if( SubsId == NULL ) {
        HandleUnlock();
        return DLNA_E_INVALID_PARAM;
    }
    if( VarName == NULL || NewVal == NULL || cVariables < 0 ) {
        HandleUnlock();
        return DLNA_E_INVALID_PARAM;
    }

    HandleUnlock();
    retVal =
        genaInitNotify( Hnd, DevID, ServName, VarName, NewVal, cVariables,
                        SubsId );

    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "Exiting dlnaAcceptSubscription \n" );
    return retVal;

}  /***************** End of dlnaAcceptSubscription *********************/

/**************************************************************************
 * Function: dlnaAcceptSubscriptionExt 
 *
 * Parameters:	
 * 	IN dlnaDevice_Handle Hnd: The handle of the device. 
 * 	IN const char *DevID: The device ID of the subdevice of the 
 *		service generating the event. 
 *	IN const char *ServID: The unique service identifier of the service 
 *		generating the event. 
 *	IN IXML_Document *PropSet: The DOM document for the property set. 
 *		Property set documents must conform to the XML schema
 *		defined in section 4.3 of the Universal Plug and Play
 *		Device Architecture specification. 
 *	IN dlna_SID SubsId: The subscription ID of the newly
 *		registered control point. 
 *
 * Description:
 *	This function is similar to dlnaAcceptSubscription except that it
 *	takes a DOM document for the variables to event rather than an array
 *	of strings. This function is sychronous and generates no callbacks.
 *
 * Return Values: int
 *	DLNA_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int
dlnaAcceptSubscriptionExt( IN dlnaDevice_Handle Hnd,
                           IN const char *DevID_const,
                           IN const char *ServName_const,
                           IN IXML_Document * PropSet,
                           IN dlna_SID SubsId )
{
    struct Handle_Info *SInfo = NULL;
    int retVal;
    char *DevID = ( char * )DevID_const;
    char *ServName = ( char * )ServName_const;

    if( dlnaSdkInit != 1 ) {
        return DLNA_E_FINISH;
    }

    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "Inside dlnaAcceptSubscription \n" );

    HandleReadLock();
    if( GetHandleInfo( Hnd, &SInfo ) != HND_DEVICE ) {
        HandleUnlock();
        return DLNA_E_INVALID_HANDLE;
    }
    if( DevID == NULL ) {
        HandleUnlock();
        return DLNA_E_INVALID_PARAM;
    }
    if( ServName == NULL ) {
        HandleUnlock();
        return DLNA_E_INVALID_PARAM;
    }
    if( SubsId == NULL ) {
        HandleUnlock();
        return DLNA_E_INVALID_PARAM;
    }

    if( PropSet == NULL ) {
        HandleUnlock();
        return DLNA_E_INVALID_PARAM;
    }

    HandleUnlock();
    retVal = genaInitNotifyExt( Hnd, DevID, ServName, PropSet, SubsId );

    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "Exiting dlnaAcceptSubscription \n" );

    return retVal;

}  /****************** End of dlnaAcceptSubscription *********************/

#endif // INCLUDE_DEVICE_APIS
#endif // EXCLUDE_GENA == 0

//---------------------------------------------------------------------------
//
//                                   SOAP interface 
//
//---------------------------------------------------------------------------
#if EXCLUDE_SOAP == 0
#ifdef INCLUDE_CLIENT_APIS

/**************************************************************************
 * Function: dlnaSendAction 
 *
 * Parameters:	
 *	IN dlnaClient_Handle Hnd: The handle of the control point 
 *		sending the action. 
 *	IN const char *ActionURL: The action URL of the service. 
 *	IN const char *ServiceType: The type of the service. 
 *	IN const char *DevUDN: This parameter is ignored. 
 *	IN IXML_Document *Action: The DOM document for the action. 
 *	OUT IXML_Document **RespNode: The DOM document for the response 
 *		to the action.  The UPnP Library allocates this document
 *		and the caller needs to free it.  
 *  
 * Description:
 *	This function sends a message to change a state variable in a service.
 *	This is a synchronous call that does not return until the action is
 *	complete.
 * 
 *	Note that a positive return value indicates a SOAP-protocol error code.
 *	In this case,  the error description can be retrieved from RespNode.
 *	A negative return value indicates a UPnP Library error.
 *
 * Return Values: int
 *	DLNA_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int
dlnaSendAction( IN dlnaClient_Handle Hnd,
                IN const char *ActionURL_const,
                IN const char *ServiceType_const,
                IN const char *DevUDN_const,
                IN IXML_Document * Action,
                OUT IXML_Document ** RespNodePtr )
{
    struct Handle_Info *SInfo = NULL;
    int retVal = 0;
    char *ActionURL = ( char * )ActionURL_const;
    char *ServiceType = ( char * )ServiceType_const;

    //char *DevUDN = (char *)DevUDN_const;  // udn not used?

    if( dlnaSdkInit != 1 ) {
        return DLNA_E_FINISH;
    }

    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "Inside dlnaSendAction \n" );
    if(DevUDN_const !=NULL) {
	dlnaPrintf(DLNA_ALL,API,__FILE__,__LINE__,"non NULL DevUDN is ignored\n");
    }
    DevUDN_const = NULL;

    HandleReadLock();
    if( GetHandleInfo( Hnd, &SInfo ) != HND_CLIENT ) {
        HandleUnlock();
        return DLNA_E_INVALID_HANDLE;
    }
    HandleUnlock();

    if( ActionURL == NULL ) {
        return DLNA_E_INVALID_PARAM;
    }

    if( ServiceType == NULL || Action == NULL || RespNodePtr == NULL
        || DevUDN_const != NULL ) {

        return DLNA_E_INVALID_PARAM;
    }

    retVal = SoapSendAction( ActionURL, ServiceType, Action, RespNodePtr );

    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "Exiting dlnaSendAction \n" );

    return retVal;

}  /****************** End of dlnaSendAction *********************/

/**************************************************************************
 * Function: dlnaSendActionEx 
 *
 * Parameters:	
 *	IN dlnaClient_Handle Hnd: The handle of the control point sending
 *		the action. 
 *	IN const char *ActionURL_const: The action URL of the service. 
 *	IN const char *ServiceType_const: The type of the service. 
 *	IN const char *DevUDN_const: This parameter is ignored. 
 *	IN IXML_Document *Header: The DOM document for the SOAP header. 
 *		This may be NULL if the header is not required. 
 *	IN IXML_Document *Action:   The DOM document for the action. 
 *	OUT IXML_Document **RespNodePtr: The DOM document for the response to
 *		the action.  The UPnP library allocates this document and the
 *		caller needs to free it.
 *  
 * Description:
 *	this function sends a message to change a state variable in a 
 *	service. This is a synchronous call that does not return until the 
 *	action is complete.
 *
 *	Note that a positive return value indicates a SOAP-protocol error code.
 *	In this case,  the error description can be retrieved from {\bf RespNode}.
 *	A negative return value indicates a UPnP Library error.
 *
 * Return Values: int
 *	DLNA_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int
dlnaSendActionEx( IN dlnaClient_Handle Hnd,
                  IN const char *ActionURL_const,
                  IN const char *ServiceType_const,
                  IN const char *DevUDN_const,
                  IN IXML_Document * Header,
                  IN IXML_Document * Action,
                  OUT IXML_Document ** RespNodePtr )
{

    struct Handle_Info *SInfo = NULL;
    int retVal = 0;
    char *ActionURL = ( char * )ActionURL_const;
    char *ServiceType = ( char * )ServiceType_const;

    //char *DevUDN = (char *)DevUDN_const;  // udn not used?

    if( dlnaSdkInit != 1 ) {
        return DLNA_E_FINISH;
    }

    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "Inside dlnaSendActionEx \n" );

    if( Header == NULL ) {
        retVal = dlnaSendAction( Hnd, ActionURL_const, ServiceType_const,
                                 DevUDN_const, Action, RespNodePtr );
        return retVal;
    }

    HandleReadLock();
    if( GetHandleInfo( Hnd, &SInfo ) != HND_CLIENT ) {
        HandleUnlock();
        return DLNA_E_INVALID_HANDLE;
    }
    HandleUnlock();

    if( ActionURL == NULL ) {
        return DLNA_E_INVALID_PARAM;
    }
    if( ServiceType == NULL || Action == NULL || RespNodePtr == NULL ) {
        return DLNA_E_INVALID_PARAM;
    }

    retVal = SoapSendActionEx( ActionURL, ServiceType, Header,
                               Action, RespNodePtr );

    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "Exiting dlnaSendAction \n" );

    return retVal;

}  /****************** End of dlnaSendActionEx *********************/

/**************************************************************************
 * Function: dlnaSendActionAsync 
 *
 *  Parameters:	
 *	IN dlnaClient_Handle Hnd: The handle of the control point 
 *		sending the action. 
 *	IN const char *ActionURL: The action URL of the service. 
 *	IN const char *ServiceType: The type of the service. 
 *	IN const char *DevUDN: This parameter is ignored. 
 *	IN IXML_Document *Action: The DOM document for the action to 
 *		perform on this device. 
 *	IN dlna_FunPtr Fun: Pointer to a callback function to 
 *		be invoked when the operation completes
 *	IN const void *Cookie: Pointer to user data that to be 
 *		passed to the callback when invoked.
 *  
 * Description:
 *	this function sends a message to change a state variable
 *	in a service, generating a callback when the operation is complete.
 *	See dlnaSendAction for comments on positive return values. These 
 *	positive return values are sent in the event struct associated with the
 *	DLNA_CONTROL_ACTION_COMPLETE event.
 *
 * Return Values: int
 *	DLNA_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int
dlnaSendActionAsync( IN dlnaClient_Handle Hnd,
                     IN const char *ActionURL_const,
                     IN const char *ServiceType_const,
                     IN const char *DevUDN_const,
                     IN IXML_Document * Act,
                     IN dlna_FunPtr Fun,
                     IN const void *Cookie_const )
{
    ThreadPoolJob job;
    struct Handle_Info *SInfo = NULL;
    struct dlnaNonblockParam *Param;
    DOMString tmpStr;
    char *ActionURL = ( char * )ActionURL_const;
    char *ServiceType = ( char * )ServiceType_const;

    //char *DevUDN = (char *)DevUDN_const;
    int rc;

    if( dlnaSdkInit != 1 ) {
        return DLNA_E_FINISH;
    }

    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "Inside dlnaSendActionAsync \n" );

    HandleReadLock();
    if( GetHandleInfo( Hnd, &SInfo ) != HND_CLIENT ) {
        HandleUnlock();
        return DLNA_E_INVALID_HANDLE;
    }
    HandleUnlock();

    if( ActionURL == NULL ) {
        return DLNA_E_INVALID_PARAM;
    }
    if( ServiceType == NULL ||
        Act == NULL || Fun == NULL || DevUDN_const != NULL ) {
        return DLNA_E_INVALID_PARAM;
    }
    tmpStr = ixmlPrintNode( ( IXML_Node * ) Act );
    if( tmpStr == NULL ) {
        return DLNA_E_INVALID_ACTION;
    }

    Param =
        ( struct dlnaNonblockParam * )
        malloc( sizeof( struct dlnaNonblockParam ) );

    if( Param == NULL ) {
        return DLNA_E_OUTOF_MEMORY;
    }

    Param->FunName = ACTION;
    Param->Handle = Hnd;
    strcpy( Param->Url, ActionURL );
    strcpy( Param->ServiceType, ServiceType );

    rc = ixmlParseBufferEx( tmpStr, &( Param->Act ) );
    if( rc != IXML_SUCCESS ) {
        free( Param );
        ixmlFreeDOMString( tmpStr );
        if( rc == IXML_INSUFFICIENT_MEMORY ) {
            return DLNA_E_OUTOF_MEMORY;
        } else {
            return DLNA_E_INVALID_ACTION;
        }
    }
    ixmlFreeDOMString( tmpStr );
    Param->Cookie = ( void * )Cookie_const;
    Param->Fun = Fun;

    TPJobInit( &job, ( start_routine ) dlnaThreadDistribution, Param );
    TPJobSetFreeFunction( &job, ( free_routine ) free );

    TPJobSetPriority( &job, MED_PRIORITY );
    ThreadPoolAdd( &gSendThreadPool, &job, NULL );

    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "Exiting dlnaSendActionAsync \n" );

    return DLNA_E_SUCCESS;

}  /****************** End of dlnaSendActionAsync *********************/

/*************************************************************************
 * Function: dlnaSendActionExAsync 
 *
 * Parameters:	
 *	IN dlnaClient_Handle Hnd: The handle of the control point 
 *		sending the action. 
 *	IN const char *ActionURL_const: The action URL of the service. 
 *	IN const char *ServiceType_const: The type of the service. 
 *	IN const char *DevUDN_const: This parameter is ignored. 
 *	IN IXML_Document *Header: The DOM document for the SOAP header. 
 *		This may be NULL if the header is not required. 
 *	IN IXML_Document *Act: The DOM document for the action to 
 *		perform on this device. 
 *	IN dlna_FunPtr Fun: Pointer to a callback function to be invoked
 *		when the operation completes. 
 *	IN const void *Cookie_const: Pointer to user data that to be
 *		passed to the callback when invoked. 
 *
 * Description:
 *	this function sends sends a message to change a state variable
 *	in a service, generating a callback when the operation is complete.
 *	See dlnaSendAction for comments on positive return values. These 
 *	positive return values are sent in the event struct associated with 
 *	the DLNA_CONTROL_ACTION_COMPLETE event.
 *
 * Return Values: int
 *	DLNA_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int
dlnaSendActionExAsync( IN dlnaClient_Handle Hnd,
                       IN const char *ActionURL_const,
                       IN const char *ServiceType_const,
                       IN const char *DevUDN_const,
                       IN IXML_Document * Header,
                       IN IXML_Document * Act,
                       IN dlna_FunPtr Fun,
                       IN const void *Cookie_const )
{
    struct Handle_Info *SInfo = NULL;
    struct dlnaNonblockParam *Param;
    DOMString tmpStr;
    DOMString headerStr = NULL;
    char *ActionURL = ( char * )ActionURL_const;
    char *ServiceType = ( char * )ServiceType_const;
    ThreadPoolJob job;
    int retVal = 0;

    if( dlnaSdkInit != 1 ) {
        return DLNA_E_FINISH;
    }

    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "Inside dlnaSendActionExAsync \n" );

    if( Header == NULL ) {
        retVal = dlnaSendActionAsync( Hnd, ActionURL_const,
                                      ServiceType_const, DevUDN_const, Act,
                                      Fun, Cookie_const );
        return retVal;
    }

    HandleReadLock();
    if( GetHandleInfo( Hnd, &SInfo ) != HND_CLIENT ) {
        HandleUnlock();
        return DLNA_E_INVALID_HANDLE;
    }
    HandleUnlock();

    if( ActionURL == NULL ) {
        return DLNA_E_INVALID_PARAM;
    }
    if( ServiceType == NULL || Act == NULL || Fun == NULL ) {
        return DLNA_E_INVALID_PARAM;
    }

    headerStr = ixmlPrintNode( ( IXML_Node * ) Header );

    tmpStr = ixmlPrintNode( ( IXML_Node * ) Act );
    if( tmpStr == NULL ) {
        return DLNA_E_INVALID_ACTION;
    }

    Param =
        ( struct dlnaNonblockParam * )
        malloc( sizeof( struct dlnaNonblockParam ) );
    if( Param == NULL ) {
        return DLNA_E_OUTOF_MEMORY;
    }

    Param->FunName = ACTION;
    Param->Handle = Hnd;
    strcpy( Param->Url, ActionURL );
    strcpy( Param->ServiceType, ServiceType );
    retVal = ixmlParseBufferEx( headerStr, &( Param->Header ) );
    if( retVal != IXML_SUCCESS ) {
        ixmlFreeDOMString( tmpStr );
        ixmlFreeDOMString( headerStr );
        if( retVal == IXML_INSUFFICIENT_MEMORY ) {
            return DLNA_E_OUTOF_MEMORY;
        } else {
            return DLNA_E_INVALID_ACTION;
        }
    }

    retVal = ixmlParseBufferEx( tmpStr, &( Param->Act ) );
    if( retVal != IXML_SUCCESS ) {
        ixmlFreeDOMString( tmpStr );
        ixmlFreeDOMString( headerStr );
        ixmlDocument_free( Param->Header );
        if( retVal == IXML_INSUFFICIENT_MEMORY ) {
            return DLNA_E_OUTOF_MEMORY;
        } else {
            return DLNA_E_INVALID_ACTION;
        }

    }

    ixmlFreeDOMString( tmpStr );
    ixmlFreeDOMString( headerStr );

    Param->Cookie = ( void * )Cookie_const;
    Param->Fun = Fun;

    TPJobInit( &job, ( start_routine ) dlnaThreadDistribution, Param );
    TPJobSetFreeFunction( &job, ( free_routine ) free );

    TPJobSetPriority( &job, MED_PRIORITY );
    ThreadPoolAdd( &gSendThreadPool, &job, NULL );

    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "Exiting dlnaSendActionAsync \n" );

    return DLNA_E_SUCCESS;

}  /****************** End of dlnaSendActionExAsync *********************/

/*************************************************************************
 * Function: dlnaGetServiceVarStatusAsync 
 *
 * Parameters:	
 *	IN dlnaClient_Handle Hnd: The handle of the control point. 
 *	IN const char *ActionURL: The URL of the service. 
 *	IN const char *VarName: The name of the variable to query. 
 *	IN dlna_FunPtr Fun: Pointer to a callback function to 
 *		be invoked when the operation is complete. 
 *	IN const void *Cookie: Pointer to user data to pass to the 
 *		callback function when invoked. 
 *
 *  Description:
 *      this function queries the state of a variable of a 
 *  service, generating a callback when the operation is complete.
 *
 *  Return Values: int
 *      DLNA_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int
dlnaGetServiceVarStatusAsync( IN dlnaClient_Handle Hnd,
                              IN const char *ActionURL_const,
                              IN const char *VarName_const,
                              IN dlna_FunPtr Fun,
                              IN const void *Cookie_const )
{
    ThreadPoolJob job;
    struct Handle_Info *SInfo = NULL;
    struct dlnaNonblockParam *Param;
    char *ActionURL = ( char * )ActionURL_const;
    char *VarName = ( char * )VarName_const;

    if( dlnaSdkInit != 1 ) {
        return DLNA_E_FINISH;
    }

    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "Inside dlnaGetServiceVarStatusAsync \n" );

    HandleReadLock();
    if( GetHandleInfo( Hnd, &SInfo ) != HND_CLIENT ) {
        HandleUnlock();
        return DLNA_E_INVALID_HANDLE;
    }
    HandleUnlock();

    if( ActionURL == NULL ) {
        return DLNA_E_INVALID_PARAM;
    }
    if( VarName == NULL || Fun == NULL )
        return DLNA_E_INVALID_PARAM;

    Param =
        ( struct dlnaNonblockParam * )
        malloc( sizeof( struct dlnaNonblockParam ) );
    if( Param == NULL ) {
        return DLNA_E_OUTOF_MEMORY;
    }

    Param->FunName = STATUS;
    Param->Handle = Hnd;
    strcpy( Param->Url, ActionURL );
    strcpy( Param->VarName, VarName );
    Param->Fun = Fun;
    Param->Cookie = ( void * )Cookie_const;

    TPJobInit( &job, ( start_routine ) dlnaThreadDistribution, Param );
    TPJobSetFreeFunction( &job, ( free_routine ) free );

    TPJobSetPriority( &job, MED_PRIORITY );

    ThreadPoolAdd( &gSendThreadPool, &job, NULL );

    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "Exiting dlnaGetServiceVarStatusAsync \n" );

    return DLNA_E_SUCCESS;

}  /****************** End of dlnaGetServiceVarStatusAsync ****************/

/**************************************************************************
 * Function: dlnaGetServiceVarStatus 
 *
 * Parameters:	
 *	IN dlnaClient_Handle Hnd: The handle of the control point.
 *	IN const char *ActionURL: The URL of the service. 
 *	IN const char *VarName: The name of the variable to query. 
 *	OUT DOMString *StVarVal: The pointer to store the value 
 *		for VarName. The UPnP Library allocates this string and
 *		the caller needs to free it.
 *  
 * Description:
 *	this function queries the state of a state variable of a service on
 *	another device.  This is a synchronous call. A positive return value
 *	indicates a SOAP error code, whereas a negative return code indicates
 *	a UPnP SDK error code.
 *
 * Return Values: int
 *	DLNA_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int
dlnaGetServiceVarStatus( IN dlnaClient_Handle Hnd,
                         IN const char *ActionURL_const,
                         IN const char *VarName_const,
                         OUT DOMString * StVar )
{
    struct Handle_Info *SInfo = NULL;
    int retVal = 0;
    char *StVarPtr;
    char *ActionURL = ( char * )ActionURL_const;
    char *VarName = ( char * )VarName_const;

    if( dlnaSdkInit != 1 ) {
        return DLNA_E_FINISH;
    }

    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "Inside dlnaGetServiceVarStatus \n" );

    HandleReadLock();
    if( GetHandleInfo( Hnd, &SInfo ) != HND_CLIENT ) {
        HandleUnlock();
        return DLNA_E_INVALID_HANDLE;
    }

    HandleUnlock();

    if( ActionURL == NULL ) {
        return DLNA_E_INVALID_PARAM;
    }
    if( VarName == NULL || StVar == NULL ) {
        return DLNA_E_INVALID_PARAM;
    }

    retVal = SoapGetServiceVarStatus( ActionURL, VarName, &StVarPtr );
    *StVar = StVarPtr;

    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "Exiting dlnaGetServiceVarStatus \n" );

    return retVal;

}  /****************** End of dlnaGetServiceVarStatus *********************/
#endif // INCLUDE_CLIENT_APIS
#endif // EXCLUDE_SOAP

//---------------------------------------------------------------------------
//
//                                   Client API's 
//
//---------------------------------------------------------------------------

/**************************************************************************
 * Function: dlnaOpenHttpPost 
 *
 * Parameters:	
 *  
 * Description:
 *
 * Return Values: int
 *	DLNA_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/

int
dlnaOpenHttpPost( IN const char *url,
                  IN OUT void **handle,
                  IN const char *contentType,
                  IN int contentLength,
                  IN int timeout )
{
    return http_OpenHttpPost( url, handle, contentType, contentLength,
                              timeout );
}

/**************************************************************************
 * Function: dlnaWriteHttpPost 
 *
 * Parameters:	
 *  
 * Description:
 *
 * Return Values: int
 *	DLNA_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int
dlnaWriteHttpPost( IN void *handle,
                   IN char *buf,
                   IN unsigned int *size,
                   IN int timeout )
{
    return http_WriteHttpPost( handle, buf, size, timeout );
}

/**************************************************************************
 * Function: dlnaCloseHttpPost 
 *
 * Parameters:	
 *  
 * Description:
 *
 * Return Values: int
 *	DLNA_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int
dlnaCloseHttpPost( IN void *handle,
                   IN OUT int *httpStatus,
                   int timeout )
{
    return http_CloseHttpPost( handle, httpStatus, timeout );
}

/**************************************************************************
 * Function: dlnaOpenHttpGet 
 *
 * Parameters:	
 *  
 * Description:
 *
 * Return Values: int
 *	DLNA_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int
dlnaOpenHttpGet( IN const char *url_str,
                 IN OUT void **Handle,
                 IN OUT char **contentType,
                 OUT int *contentLength,
                 OUT int *httpStatus,
                 IN int timeout )
{
    return http_OpenHttpGet( url_str, Handle, contentType, contentLength,
                             httpStatus, timeout );
}



/**************************************************************************
 * Function: dlnaOpenHttpGetProxy
 *
 * Parameters:	
 *  
 * Description:
 *
 * Return Values: int
 *	DLNA_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int
dlnaOpenHttpGetProxy( IN const char *url_str,
                 IN const char *proxy_str,
                 IN OUT void **Handle,
                 IN OUT char **contentType,
                 OUT int *contentLength,
                 OUT int *httpStatus,
                 IN int timeout )
{
    return http_OpenHttpGetProxy( url_str, proxy_str, Handle, contentType, contentLength,
                             httpStatus, timeout );
}

/**************************************************************************
 * Function: dlnaOpenHttpGetEx
 *
 * Parameters:	
 *  
 * Description:
 *
 * Return Values: int
 *	DLNA_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int
dlnaOpenHttpGetEx( IN const char *url_str,
                   IN OUT void **Handle,
                   IN OUT char **contentType,
                   OUT int *contentLength,
                   OUT int *httpStatus,
                   IN int lowRange,
                   IN int highRange,
                   IN int timeout )
{
    return http_OpenHttpGetEx( url_str,
                               Handle,
                               contentType,
                               contentLength,
                               httpStatus, lowRange, highRange, timeout );
}



/**************************************************************************
 * Function: dlnaCancelHttpGet 
 *
 * Parameters:	
 *  
 * Description:
 *
 * Return Values: int
 *	DLNA_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int
dlnaCancelHttpGet( IN void *Handle )
{
    return http_CancelHttpGet( Handle );
}

/**************************************************************************
 * Function: dlnaCloseHttpGet 
 *
 * Parameters:	
 *  
 * Description:
 *
 * Return Values: int
 *	DLNA_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int
dlnaCloseHttpGet( IN void *Handle )
{
    return http_CloseHttpGet( Handle );
}

/**************************************************************************
 * Function: dlnaReadHttpGet 
 *
 * Parameters:	
 *  
 * Description:
 *
 * Return Values: int
 *	DLNA_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int
dlnaReadHttpGet( IN void *Handle,
                 IN OUT char *buf,
                 IN OUT unsigned int *size,
                 IN int timeout )
{
    return http_ReadHttpGet( Handle, buf, size, timeout );
}



/**************************************************************************
 * Function: dlnaHttpGetProgress 
 *
 * Parameters:	
 *  
 * Description:
 *
 * Return Values: int
 *	DLNA_E_SUCCESS if successful.
 *	DLNA_E_INVALID_PARAM if the provided pointers were invalid.
 ***************************************************************************/
int
dlnaHttpGetProgress( IN void *Handle, 
                     OUT unsigned int *length,
                     OUT unsigned int *total )
{
    return http_HttpGetProgress(Handle, length, total);
}

/**************************************************************************
 * Function: dlnaDownloadUrlItem 
 *
 * Parameters:	
 *  
 * Description:
 *
 * Return Values: int
 *	DLNA_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int
dlnaDownloadUrlItem( const char *url,
                     char **outBuf,
                     char *contentType )
{
    int ret_code;
    int dummy;

    if( url == NULL || outBuf == NULL || contentType == NULL ) {
        return DLNA_E_INVALID_PARAM;
    }

    ret_code = http_Download( url, HTTP_DEFAULT_TIMEOUT, outBuf, &dummy,
                              contentType );
    if( ret_code > 0 ) {
        // error reply was received
        ret_code = DLNA_E_INVALID_URL;
    }

    return ret_code;
}

/**************************************************************************
 * Function: dlnaDownloadXmlDoc 
 *
 * Parameters:	
 *  
 * Description:
 *
 * Return Values: int
 *	DLNA_E_SUCCESS if successful else sends appropriate error.
 ***************************************************************************/
int
dlnaDownloadXmlDoc( const char *url,
                    IXML_Document ** xmlDoc )
{
    int ret_code;
    char *xml_buf;
    char content_type[LINE_SIZE];

    if( url == NULL || xmlDoc == NULL ) {
        return DLNA_E_INVALID_PARAM;
    }

    ret_code = dlnaDownloadUrlItem( url, &xml_buf, content_type );
    if( ret_code != DLNA_E_SUCCESS ) {
        dlnaPrintf( DLNA_CRITICAL, API, __FILE__, __LINE__,
            "retCode: %d\n", ret_code );
        return ret_code;
    }

    if( strncasecmp( content_type, "text/xml", strlen( "text/xml" ) ) ) {
        dlnaPrintf( DLNA_INFO, API, __FILE__, __LINE__, "Not text/xml\n" );
        // Linksys WRT54G router returns 
        // "CONTENT-TYPE: application/octet-stream".
        // Let's be nice to Linksys and try to parse document anyway.
        // If the data sended is not a xml file, ixmlParseBufferEx
        // will fail and the function will return DLNA_E_INVALID_DESC too.
#if 0
        free( xml_buf );
        return DLNA_E_INVALID_DESC;
#endif
    }

    ret_code = ixmlParseBufferEx( xml_buf, xmlDoc );
    free( xml_buf );

    if( ret_code != IXML_SUCCESS ) {
        dlnaPrintf( DLNA_CRITICAL, API, __FILE__, __LINE__,
            "Invalid desc\n" );
        if( ret_code == IXML_INSUFFICIENT_MEMORY ) {
            return DLNA_E_OUTOF_MEMORY;
        } else {
            return DLNA_E_INVALID_DESC;
        }
    } else {
#ifdef DEBUG
        xml_buf = ixmlPrintNode( ( IXML_Node * ) * xmlDoc );
        dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
            "Printing the Parsed xml document \n %s\n", xml_buf );
        dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
            "****************** END OF Parsed XML Doc *****************\n" );
        ixmlFreeDOMString( xml_buf );
#endif
        dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
            "Exiting dlnaDownloadXmlDoc\n" );
        return DLNA_E_SUCCESS;
    }
}

//----------------------------------------------------------------------------
//
//                          DLNA-API  Internal function implementation
//
//----------------------------------------------------------------------------


/**************************************************************************
 * Function: upnp.hreadDistribution 
 *
 * Parameters:	
 *  
 * Description:
 *	Function to schedule async functions in threadpool.
 *
 * Return Values: VOID
 *      
 ***************************************************************************/
#ifdef INCLUDE_CLIENT_APIS
void
dlnaThreadDistribution( struct dlnaNonblockParam *Param )
{

    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "Inside upnp.hreadDistribution \n" );

    switch ( Param->FunName ) {
#if EXCLUDE_GENA == 0
        case SUBSCRIBE: {
            struct dlna_Event_Subscribe Evt;
            Evt.ErrCode = genaSubscribe(
                Param->Handle, Param->Url,
                ( int * )&( Param->TimeOut ),
                ( char * )Evt.Sid );
            strcpy( Evt.PublisherUrl, Param->Url );
            Evt.TimeOut = Param->TimeOut;
            Param->Fun( DLNA_EVENT_SUBSCRIBE_COMPLETE, &Evt, Param->Cookie );
            free( Param );
            break;
        }
        case UNSUBSCRIBE: {
	    struct dlna_Event_Subscribe Evt;
	    Evt.ErrCode =
	    genaUnSubscribe( Param->Handle,
			     Param->SubsId );
	    strcpy( ( char * )Evt.Sid, Param->SubsId );
	    strcpy( Evt.PublisherUrl, "" );
	    Evt.TimeOut = 0;
	    Param->Fun( DLNA_EVENT_UNSUBSCRIBE_COMPLETE,
			&Evt, Param->Cookie );
	    free( Param );
            break;
        }
        case RENEW: {
	    struct dlna_Event_Subscribe Evt;
	    Evt.ErrCode =
	    genaRenewSubscription( Param->Handle,
				   Param->SubsId,
				   &( Param->TimeOut ) );
	    Evt.TimeOut = Param->TimeOut;
	    strcpy( ( char * )Evt.Sid, Param->SubsId );
	    Param->Fun( DLNA_EVENT_RENEWAL_COMPLETE, &Evt,
			Param->Cookie );
            free( Param );
	    break;
        }
#endif // EXCLUDE_GENA == 0
#if EXCLUDE_SOAP == 0
        case ACTION: {
            struct dlna_Action_Complete Evt;
            Evt.ActionResult = NULL;
                Evt.ErrCode =
                    SoapSendAction( Param->Url, Param->ServiceType,
                                    Param->Act, &Evt.ActionResult );
                Evt.ActionRequest = Param->Act;
                strcpy( Evt.CtrlUrl, Param->Url );
                Param->Fun( DLNA_CONTROL_ACTION_COMPLETE, &Evt,
                            Param->Cookie );
                ixmlDocument_free( Evt.ActionRequest );
                ixmlDocument_free( Evt.ActionResult );
                free( Param );
                break;
        }
        case STATUS: {
                struct dlna_State_Var_Complete Evt;
                Evt.ErrCode = SoapGetServiceVarStatus(
                    Param->Url, Param->VarName, &( Evt.CurrentVal ) );
                strcpy( Evt.StateVarName, Param->VarName );
                strcpy( Evt.CtrlUrl, Param->Url );
                Param->Fun( DLNA_CONTROL_GET_VAR_COMPLETE, &Evt,
                            Param->Cookie );
                free( Evt.CurrentVal );
                free( Param );
                break;
            }
#endif // EXCLUDE_SOAP == 0
        default:
            break;
    } // end of switch(Param->FunName)

    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "Exiting upnp.hreadDistribution \n" );

}  /****************** End of upnp.hreadDistribution  *********************/
#endif // INCLUDE_CLIENT_APIS

/**************************************************************************
 * Function: GetCallBackFn 
 *
 * Parameters:	
 *  
 * Description:
 *	This function is to get callback function ptr from a handle
 *
 * Return Values: dlna_FunPtr
 *      
 ***************************************************************************/
dlna_FunPtr
GetCallBackFn( dlnaClient_Handle Hnd )
{
    return ( ( struct Handle_Info * )HandleTable[Hnd] )->Callback;

}  /****************** End of GetCallBackFn *********************/

/**************************************************************************
 * Function: InitHandleList 
 *
 * Parameters: VOID
 *  
 * Description:
 *	This function is to initialize handle table
 *
 * Return Values: VOID
 *      
 ***************************************************************************/
void
InitHandleList()
{
    int i;

    for( i = 0; i < NUM_HANDLE; i++ )
        HandleTable[i] = NULL;

}  /****************** End of InitHandleList *********************/

/**************************************************************************
 * Function: GetFreeHandle 
 *
 * Parameters: VOID
 *  
 * Description:
 *	This function is to get a free handle
 *
 * Return Values: VOID
 *      
 ***************************************************************************/
int
GetFreeHandle()
{
    int i = 1;

    /*
       Handle 0 is not used as NULL translates to 0 when passed as a handle 
     */
    while( i < NUM_HANDLE ) {
        if( HandleTable[i++] == NULL )
            break;
    }

    if( i == NUM_HANDLE )
        return DLNA_E_OUTOF_HANDLE; //Error
    else
        return --i;

}  /****************** End of GetFreeHandle *********************/

/**************************************************************************
 * Function: GetClientHandleInfo 
 *
 * Parameters:	
 *	IN dlnaClient_Handle *client_handle_out: client handle pointer ( key 
 *		for the client handle structure).
 *	OUT struct Handle_Info **HndInfo: Client handle structure passed by 
 *		this function.
 *
 * Description:
 *	This function is to get client handle info
 *
 *  Return Values: HND_CLIENT
 *      
 ***************************************************************************/
//Assumes at most one client
dlna_Handle_Type
GetClientHandleInfo( IN dlnaClient_Handle * client_handle_out,
                     OUT struct Handle_Info ** HndInfo )
{
    ( *client_handle_out ) = 1;
    if( GetHandleInfo( 1, HndInfo ) == HND_CLIENT ) {
        return HND_CLIENT;
    }
    ( *client_handle_out ) = 2;
    if( GetHandleInfo( 2, HndInfo ) == HND_CLIENT ) {
        return HND_CLIENT;
    }
    ( *client_handle_out ) = -1;
    return HND_INVALID;

}  /****************** End of GetClientHandleInfo *********************/

/**************************************************************************
 * Function: GetDeviceHandleInfo 
 *
 * Parameters:	
 * 	IN dlnaDevice_Handle * device_handle_out: device handle pointer
 * 		(key for the client handle structure).
 *	OUT struct Handle_Info **HndInfo: Device handle structure passed by
 *		this function.
 *  
 *  Description:
 *		This function is to get device handle info.
 *
 *  Return Values: HND_DEVICE
 *      
 ***************************************************************************/
dlna_Handle_Type
GetDeviceHandleInfo( dlnaDevice_Handle * device_handle_out,
                     struct Handle_Info ** HndInfo )
{
    ( *device_handle_out ) = 1;
    if( GetHandleInfo( 1, HndInfo ) == HND_DEVICE )
        return HND_DEVICE;

    ( *device_handle_out ) = 2;
    if( GetHandleInfo( 2, HndInfo ) == HND_DEVICE )
        return HND_DEVICE;
    ( *device_handle_out ) = -1;

    return HND_INVALID;

}  /****************** End of GetDeviceHandleInfo *********************/

/**************************************************************************
 * Function: GetDeviceHandleInfo 
 *
 * Parameters:	
 * 	IN dlnaClient_Handle * device_handle_out: handle pointer
 * 		(key for the client handle structure).
 *	OUT struct Handle_Info **HndInfo: handle structure passed by
 *		this function.
 *  
 * Description:
 *	This function is to get  handle info.
 *
 * Return Values: HND_DEVICE
 *      
 ***************************************************************************/
dlna_Handle_Type
GetHandleInfo( dlnaClient_Handle Hnd,
               struct Handle_Info ** HndInfo )
{

    dlnaPrintf( DLNA_INFO, API, __FILE__, __LINE__,
        "GetHandleInfo: Handle is %d\n", Hnd );

    if( Hnd < 1 || Hnd >= NUM_HANDLE ) {
        dlnaPrintf( DLNA_INFO, API, __FILE__, __LINE__,
            "GetHandleInfo : Handle out of range\n" );
        return DLNA_E_INVALID_HANDLE;
    }
    if( HandleTable[Hnd] != NULL ) {
        *HndInfo = ( struct Handle_Info * )HandleTable[Hnd];
        return ( ( struct Handle_Info * )*HndInfo )->HType;
    }
    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "GetHandleInfo : exiting\n" );

    return DLNA_E_INVALID_HANDLE;

}  /****************** End of GetHandleInfo *********************/

/**************************************************************************
 * Function: FreeHandle 
 *
 * Parameters:	
 * 	IN int dlna_Handle: handle index 
 *  
 * Description:
 *	This function is to to free handle info.
 *	
 * Return Values: int
 *	DLNA_E_SUCCESS if successful else return appropriate error
 ***************************************************************************/
int
FreeHandle( int dlna_Handle )
{
    if( dlna_Handle < 1 || dlna_Handle >= NUM_HANDLE ) {
        dlnaPrintf( DLNA_CRITICAL, API, __FILE__, __LINE__,
            "FreeHandleInfo : Handle out of range\n" );
        return DLNA_E_INVALID_HANDLE;
    }

    if( HandleTable[dlna_Handle] == NULL ) {
        return DLNA_E_INVALID_HANDLE;
    }
    free( HandleTable[dlna_Handle] );
    HandleTable[dlna_Handle] = NULL;
    return DLNA_E_SUCCESS;

}  /****************** End of FreeHandle *********************/


/**************************************************************************
 * Function: PrintHandleInfo 
 *
 * Parameters:	
 *	IN dlnaClient_Handle Hnd: handle index 
 *  
 * Description:
 *	This function is to print handle info.
 *	
 * Return Values: int
 *	DLNA_E_SUCCESS if successful else return appropriate error
 ***************************************************************************/
int PrintHandleInfo( IN dlnaClient_Handle Hnd )
{
    struct Handle_Info * HndInfo;
    if (HandleTable[Hnd] != NULL) {
        HndInfo = HandleTable[Hnd];
            dlnaPrintf(DLNA_ALL, API, __FILE__, __LINE__,
                "Printing information for Handle_%d\n", Hnd);
            dlnaPrintf(DLNA_ALL, API, __FILE__, __LINE__,
                "HType_%d\n", HndInfo->HType);
#ifdef INCLUDE_DEVICE_APIS
                if(HndInfo->HType != HND_CLIENT)
                    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
                        "DescURL_%s\n", HndInfo->DescURL );
#endif
    } else {
        return DLNA_E_INVALID_HANDLE;
    }

    return DLNA_E_SUCCESS;
}

   /****************** End of PrintHandleInfo *********************/

void printNodes( IXML_Node * tmpRoot, int depth )
{
    int i;
    IXML_NodeList *NodeList1;
    IXML_Node *ChildNode1;
    unsigned short NodeType;
    const DOMString NodeValue;
    const DOMString NodeName;
    NodeList1 = ixmlNode_getChildNodes(tmpRoot);
    for (i = 0; i < 100; ++i) {
        ChildNode1 = ixmlNodeList_item(NodeList1, i);
        if (ChildNode1 == NULL) {
            break;
        }
    
        printNodes(ChildNode1, depth+1);
        NodeType = ixmlNode_getNodeType(ChildNode1);
        NodeValue = ixmlNode_getNodeValue(ChildNode1);
        NodeName = ixmlNode_getNodeName(ChildNode1);
        dlnaPrintf(DLNA_ALL, API, __FILE__, __LINE__,
            "DEPTH-%2d-IXML_Node Type %d, "
            "IXML_Node Name: %s, IXML_Node Value: %s\n",
            depth, NodeType, NodeName, NodeValue);
    }
}

   /****************** End of printNodes *********************/

    //********************************************************
    //* Name: getlocalhostname
    //* Description:  Function to get local IP address
    //*               Gets the ip address for the DEFAULT_INTERFACE 
    //*               interface which is up and not a loopback
    //*               assumes at most MAX_INTERFACES interfaces
    //* Called by:    dlnaInit
    //* In:           char *out
    //* Out:          Ip address
    //* Return codes: DLNA_E_SUCCESS
    //* Error codes:  DLNA_E_INIT
    //********************************************************

 /**************************************************************************
 * Function: getlocalhostname 
 *
 * Parameters:	
 * 	OUT char *out: IP address of the interface.
 *  
 * Description:
 *	This function is to get local IP address. It gets the ip address for 
 *	the DEFAULT_INTERFACE interface which is up and not a loopback
 *	assumes at most MAX_INTERFACES interfaces
 *
 *  Return Values: int
 *	DLNA_E_SUCCESS if successful else return appropriate error
 ***************************************************************************/
    int getlocalhostname( OUT char *out ) {

#ifdef WIN32
 	 struct hostent *h=NULL;
    struct sockaddr_in LocalAddr;

 		gethostname(out,LINE_SIZE);
 		h=gethostbyname(out);
 		if (h!=NULL){
 			memcpy(&LocalAddr.sin_addr,h->h_addr_list[0],4);
 			strcpy( out, inet_ntoa(LocalAddr.sin_addr));
 		}
 		return DLNA_E_SUCCESS;
#elif (defined(BSD) && BSD >= 199306)
    struct ifaddrs *ifap, *ifa;

    if (getifaddrs(&ifap) != 0) {
        dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
            "DiscoverInterfaces: getifaddrs() returned error\n" );
        return DLNA_E_INIT;
    }

    // cycle through available interfaces
    for (ifa = ifap; ifa != NULL; ifa = ifa->ifa_next) {
        // Skip loopback, point-to-point and down interfaces, 
        // except don't skip down interfaces
        // if we're trying to get a list of configurable interfaces. 
        if( ( ifa->ifa_flags & IFF_LOOPBACK )
            || ( !( ifa->ifa_flags & IFF_UP ) ) ) {
            continue;
        }
        if( ifa->ifa_addr->sa_family == AF_INET ) {
            // We don't want the loopback interface. 
            if( ((struct sockaddr_in *)(ifa->ifa_addr))->sin_addr.s_addr ==
                htonl( INADDR_LOOPBACK ) ) {
                continue;
            }

            strncpy( out, inet_ntoa( ((struct sockaddr_in *)(ifa->ifa_addr))->
                sin_addr ), LINE_SIZE );
            out[LINE_SIZE-1] = '\0';
            dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
                "Inside getlocalhostname : after strncpy %s\n",
                out );
            break;
        }
    }
    freeifaddrs(ifap);

    return ifa ? DLNA_E_SUCCESS : DLNA_E_INIT;
#else
    char szBuffer[MAX_INTERFACES * sizeof( struct ifreq )];
    struct ifconf ifConf;
    struct ifreq ifReq;
    int nResult;
    int i;
    int LocalSock;
    struct sockaddr_in LocalAddr;
    int j = 0;

    // Create an unbound datagram socket to do the SIOCGIFADDR ioctl on. 
    if( ( LocalSock = socket( AF_INET, SOCK_DGRAM, IPPROTO_UDP ) ) < 0 ) {
        dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
            "Can't create addrlist socket\n" );
        return DLNA_E_INIT;
    }
    // Get the interface configuration information... 
    ifConf.ifc_len = sizeof szBuffer;
    ifConf.ifc_ifcu.ifcu_buf = ( caddr_t ) szBuffer;
    nResult = ioctl( LocalSock, SIOCGIFCONF, &ifConf );

    if( nResult < 0 ) {
        dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
            "DiscoverInterfaces: SIOCGIFCONF returned error\n" );

        return DLNA_E_INIT;
    }
    // Cycle through the list of interfaces looking for IP addresses. 

    for( i = 0; ( ( i < ifConf.ifc_len ) && ( j < DEFAULT_INTERFACE ) ); ) {
        struct ifreq *pifReq =
            ( struct ifreq * )( ( caddr_t ) ifConf.ifc_req + i );
        i += sizeof *pifReq;

        // See if this is the sort of interface we want to deal with.
        strcpy( ifReq.ifr_name, pifReq->ifr_name );
        if( ioctl( LocalSock, SIOCGIFFLAGS, &ifReq ) < 0 ) {
            dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
                "Can't get interface flags for %s:\n",
                ifReq.ifr_name );
        }
        // Skip loopback, point-to-point and down interfaces, 
        // except don't skip down interfaces
        // if we're trying to get a list of configurable interfaces. 
        if( ( ifReq.ifr_flags & IFF_LOOPBACK )
            || ( !( ifReq.ifr_flags & IFF_UP ) ) ) {
            continue;
        }
        if( pifReq->ifr_addr.sa_family == AF_INET ) {
            // Get a pointer to the address...
            memcpy( &LocalAddr, &pifReq->ifr_addr,
                    sizeof pifReq->ifr_addr );

            // We don't want the loopback interface. 
            if( LocalAddr.sin_addr.s_addr == htonl( INADDR_LOOPBACK ) ) {
                continue;
            }

        }
        //increment j if we found an address which is not loopback
        //and is up
        j++;

    }
    close( LocalSock );

    strncpy( out, inet_ntoa( LocalAddr.sin_addr ), LINE_SIZE );

    dlnaPrintf( DLNA_ALL, API, __FILE__, __LINE__,
        "Inside getlocalhostname : after strncpy %s\n",
        out );
    return DLNA_E_SUCCESS;
#endif
    }

#ifdef INCLUDE_DEVICE_APIS
#if EXCLUDE_SSDP == 0

 /**************************************************************************
 * Function: AutoAdvertise 
 *
 * Parameters:	
 * 	IN void *input: information provided to the thread.
 *  
 * Description:
 *	This function is a timer thread scheduled by dlnaSendAdvertisement 
 *	to the send advetisement again. 
 *
 * Return Values: VOID
 *     
 ***************************************************************************/
void
AutoAdvertise( void *input )
{
    dlna_timeout *event = ( dlna_timeout * ) input;

    dlnaSendAdvertisement( event->handle, *( ( int * )event->Event ) );
    free_dlna_timeout( event );
}
#endif //INCLUDE_DEVICE_APIS
#endif

/*
 **************************** */
#ifdef INTERNAL_WEB_SERVER

 /**************************************************************************
 * Function: dlnaSetWebServerRootDir 
 *
 * Parameters:	
 *	IN const char* rootDir:Path of the root directory of the web server. 
 *  
 * Description:
 *	This function sets the document root directory for
 *	the internal web server. This directory is considered the
 *	root directory (i.e. "/") of the web server.
 *	This function also activates or deactivates the web server.
 *	To disable the web server, pass NULL for rootDir to 
 *	activate, pass a valid directory string.
 *  
 *	Note that this function is not available when the web server is not
 *	compiled into the UPnP Library.
 *
 * Return Values: int
 *	DLNA_E_SUCCESS if successful else returns appropriate error
 ***************************************************************************/
int
dlnaSetWebServerRootDir( IN const char *rootDir )
{
    if( dlnaSdkInit == 0 )
        return DLNA_E_FINISH;
    if( ( rootDir == NULL ) || ( strlen( rootDir ) == 0 ) ) {
        return DLNA_E_INVALID_PARAM;
    }

    membuffer_destroy( &gDocumentRootDir );

    return ( web_server_set_root_dir( rootDir ) );
}
#endif // INTERNAL_WEB_SERVER
/*
 *************************** */

 /**************************************************************************
 * Function: dlnaAddVirtualDir 
 *
 * Parameters:	
 *	IN const char *newDirName:The name of the new directory mapping to add.
 *  
 * Description:
 *	This function adds a virtual directory mapping.
 *
 *	All webserver requests containing the given directory are read using
 *	functions contained in a dlnaVirtualDirCallbacks structure registered
 *	via dlnaSetVirtualDirCallbacks.
 *  
 *	Note that this function is not available when the web server is not
 *	compiled into the UPnP Library.
 *
 *  Return Values: int
 *     DLNA_E_SUCCESS if successful else returns appropriate error
 ***************************************************************************/
int
dlnaAddVirtualDir( IN const char *newDirName )
{

    virtualDirList *pNewVirtualDir,
     *pLast;
    virtualDirList *pCurVirtualDir;
    char dirName[NAME_SIZE];

    if( dlnaSdkInit != 1 ) {
        // SDK is not initialized
        return DLNA_E_FINISH;
    }

    if( ( newDirName == NULL ) || ( strlen( newDirName ) == 0 ) ) {
        return DLNA_E_INVALID_PARAM;
    }

    if( *newDirName != '/' ) {
        dirName[0] = '/';
        strcpy( dirName + 1, newDirName );
    } else {
        strcpy( dirName, newDirName );
    }

    pCurVirtualDir = pVirtualDirList;
    while( pCurVirtualDir != NULL ) {
        // already has this entry
        if( strcmp( pCurVirtualDir->dirName, dirName ) == 0 ) {
            return DLNA_E_SUCCESS;
        }

        pCurVirtualDir = pCurVirtualDir->next;
    }

    pNewVirtualDir =
        ( virtualDirList * ) malloc( sizeof( virtualDirList ) );
    if( pNewVirtualDir == NULL ) {
        return DLNA_E_OUTOF_MEMORY;
    }
    pNewVirtualDir->next = NULL;
    strcpy( pNewVirtualDir->dirName, dirName );
    *( pNewVirtualDir->dirName + strlen( dirName ) ) = 0;

    if( pVirtualDirList == NULL ) { // first virtual dir
        pVirtualDirList = pNewVirtualDir;
    } else {
        pLast = pVirtualDirList;
        while( pLast->next != NULL ) {
            pLast = pLast->next;
        }
        pLast->next = pNewVirtualDir;
    }

    return DLNA_E_SUCCESS;
}

 /**************************************************************************
 * Function: dlnaRemoveVirtualDir 
 *
 * Parameters:	
 * 	IN const char *newDirName:The name of the directory mapping to remove.
 *  
 * Description:
 *	This function removes a virtual directory mapping.
 *
 * Return Values: int
 *	DLNA_E_SUCCESS if successful else returns appropriate error
 ***************************************************************************/
int
dlnaRemoveVirtualDir( IN const char *dirName )
{

    virtualDirList *pPrev;
    virtualDirList *pCur;
    int found = 0;

    if( dlnaSdkInit != 1 ) {
        return DLNA_E_FINISH;
    }

    if( dirName == NULL ) {
        return DLNA_E_INVALID_PARAM;
    }

    if( pVirtualDirList == NULL ) {
        return DLNA_E_INVALID_PARAM;
    }
    //
    // Handle the special case where the directory that we are
    // removing is the first and only one in the list.
    //

    if( ( pVirtualDirList->next == NULL ) &&
        ( strcmp( pVirtualDirList->dirName, dirName ) == 0 ) ) {
        free( pVirtualDirList );
        pVirtualDirList = NULL;
        return DLNA_E_SUCCESS;
    }

    pCur = pVirtualDirList;
    pPrev = pCur;

    while( pCur != NULL ) {
        if( strcmp( pCur->dirName, dirName ) == 0 ) {
            pPrev->next = pCur->next;
            free( pCur );
            found = 1;
            break;
        } else {
            pPrev = pCur;
            pCur = pCur->next;
        }
    }

    if( found == 1 )
        return DLNA_E_SUCCESS;
    else
        return DLNA_E_INVALID_PARAM;

}

 /**************************************************************************
 * Function: dlnaRemoveAllVirtualDirs 
 *
 * Parameters: VOID
 *  
 * Description:
 *	This function removes all the virtual directory mappings.
 *
 * Return Values: VOID
 *     
 ***************************************************************************/
void
dlnaRemoveAllVirtualDirs()
{

    virtualDirList *pCur;
    virtualDirList *pNext;

    if( dlnaSdkInit != 1 ) {
        return;
    }

    pCur = pVirtualDirList;

    while( pCur != NULL ) {
        pNext = pCur->next;
        free( pCur );

        pCur = pNext;
    }

    pVirtualDirList = NULL;

}

 /**************************************************************************
 * Function: dlnaEnableWebserver 
 *
 * Parameters:	
 *	IN int enable: TRUE to enable, FALSE to disable.
 *  
 * Description:
 *	This function enables or disables the webserver.  A value of
 *	TRUE enables the webserver, FALSE disables it.
 *
 * Return Values: int
 *	DLNA_E_SUCCESS if successful else returns appropriate error
 ***************************************************************************/
int
dlnaEnableWebserver( IN int enable )
{
    int retVal;

    if( dlnaSdkInit != 1 ) {
        return DLNA_E_FINISH;
    }

    switch ( enable ) {
#ifdef INTERNAL_WEB_SERVER
        case TRUE:
            if( ( retVal = web_server_init() ) != DLNA_E_SUCCESS ) {
                return retVal;
            }
            bWebServerState = WEB_SERVER_ENABLED;
            SetHTTPGetCallback( web_server_callback );
            break;

        case FALSE:
            web_server_destroy();
            bWebServerState = WEB_SERVER_DISABLED;
            SetHTTPGetCallback( NULL );
            break;
#endif
        default:
            return DLNA_E_INVALID_PARAM;
    }

    return DLNA_E_SUCCESS;
}

 /**************************************************************************
 * Function: dlnaIsWebserverEnabled 
 *
 * Parameters: VOID
 *  
 * Description:
 *	This function  checks if the webserver is enabled or disabled. 
 *
 * Return Values: int
 *	1, if webserver enabled
 *	0, if webserver disabled
 ***************************************************************************/
int
dlnaIsWebserverEnabled()
{
    if( dlnaSdkInit != 1 ) {
        return 0;
    }

    return ( bWebServerState == WEB_SERVER_ENABLED );
}

 /**************************************************************************
 * Function: dlnaSetVirtualDirCallbacks 
 *
 * Parameters:	
 *	IN struct dlnaVirtualDirCallbacks *callbacks:a structure that 
 *		contains the callback functions.
 *	
 * Description:
 *	This function sets the callback function to be used to 
 *	access a virtual directory.
 *
 * Return Values: int
 *	DLNA_E_SUCCESS on success, or DLNA_E_INVALID_PARAM
 ***************************************************************************/
int
dlnaSetVirtualDirCallbacks( IN struct dlnaVirtualDirCallbacks *callbacks, IN void *cookie )
{
    struct dlnaVirtualDirCallbacks *pCallback;

    if( dlnaSdkInit != 1 ) {
        // SDK is not initialized
        return DLNA_E_FINISH;
    }

    pCallback = &virtualDirCallback;

    if( callbacks == NULL )
        return DLNA_E_INVALID_PARAM;

    pCallback->cookie = cookie;
    pCallback->get_info = callbacks->get_info;
    pCallback->open = callbacks->open;
    pCallback->close = callbacks->close;
    pCallback->read = callbacks->read;
    pCallback->write = callbacks->write;
    pCallback->seek = callbacks->seek;

    return DLNA_E_SUCCESS;
}

 /**************************************************************************
 * Function: dlnaFree 
 *
 * Parameters:	
 *	IN void *item:The item to free.
 *	
 * Description:
 *	This function free the memory allocated by tbe UPnP library
 *
 * Return Values: VOID
 *		
 ***************************************************************************/
void
dlnaFree( IN void *item )
{
    if( item )
        free( item );
}


/**************************************************************************
 * Function: dlnaSetContentLength
 * OBSOLETE METHOD : use {\bf dlnaSetMaxContentLength} instead.
 ***************************************************************************/
int
dlnaSetContentLength( IN dlnaClient_Handle Hnd,
                               /** The handle of the device instance
                                  for which the coincoming content length needs
                                  to be set. */

                      IN int contentLength
                               /** Permissible content length  */
     )
{
    int errCode = DLNA_E_SUCCESS;
    struct Handle_Info *HInfo = NULL;

    do {
        if( dlnaSdkInit != 1 ) {
            errCode = DLNA_E_FINISH;
            break;
        }

        HandleLock();

        errCode = GetHandleInfo( Hnd, &HInfo );

        if( errCode != HND_DEVICE ) {
            errCode = DLNA_E_INVALID_HANDLE;
            break;
        }

        if( contentLength > MAX_SOAP_CONTENT_LENGTH ) {
            errCode = DLNA_E_OUTOF_BOUNDS;
            break;
        }

        g_maxContentLength = contentLength;

    } while( 0 );

    HandleUnlock();
    return errCode;

}


/**************************************************************************
 * Function: dlnaSetMaxContentLength
 *
 * Parameters:	
 *	IN int contentLength: The maximum size to be set 
 *	
 * Description:
 *	Sets the maximum content-length that the SDK will process on an 
 *	incoming SOAP requests or responses. This API allows devices that have
 *	memory constraints to exhibit consistent behaviour if the size of the 
 *	incoming SOAP message exceeds the memory that device can allocate. 
 *	The default maximum content-length is {\tt DEFAULT_SOAP_CONTENT_LENGTH}
 *	= 16K bytes.
 *
 * Return Values: int
 *	DLNA_E_SUCCESS: The operation completed successfully.
 *		
 ***************************************************************************/
int
dlnaSetMaxContentLength (
                      IN size_t contentLength
                               /** Permissible content length, in bytes  */
     )
{
    int errCode = DLNA_E_SUCCESS;

    do {
        if( dlnaSdkInit != 1 ) {
            errCode = DLNA_E_FINISH;
            break;
        }

        g_maxContentLength = contentLength;

    } while( 0 );

    return errCode;

}

/*********************** END OF FILE dlnaapi.c :) ************************/
