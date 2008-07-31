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
#if EXCLUDE_DOM == 0
#include <stdarg.h>
#include "upnptools.h"
#include "uri.h"
#define HEADER_LENGTH 2000

// Structure to maintain a error code and string associated with the 
// error code
struct ErrorString {
    int rc;                     /* error code */
    const char *rcError;        /* error description */

};

// Initializing the array of error structures. 
struct ErrorString ErrorMessages[] = { {DLNA_E_SUCCESS, "DLNA_E_SUCCESS"},
{DLNA_E_INVALID_HANDLE, "DLNA_E_INVALID_HANDLE"},
{DLNA_E_INVALID_PARAM, "DLNA_E_INVALID_PARAM"},
{DLNA_E_OUTOF_HANDLE, "DLNA_E_OUTOF_HANDLE"},
{DLNA_E_OUTOF_CONTEXT, "DLNA_E_OUTOF_CONTEXT"},
{DLNA_E_OUTOF_MEMORY, "DLNA_E_OUTOF_MEMOR"},
{DLNA_E_INIT, "DLNA_E_INIT"},
{DLNA_E_BUFFER_TOO_SMALL, "DLNA_E_BUFFER_TOO_SMALL"},
{DLNA_E_INVALID_DESC, "DLNA_E_INVALID_DESC"},
{DLNA_E_INVALID_URL, "DLNA_E_INVALID_URL"},
{DLNA_E_INVALID_SID, "DLNA_E_INVALID_SID"},
{DLNA_E_INVALID_DEVICE, "DLNA_E_INVALID_DEVICE"},
{DLNA_E_INVALID_SERVICE, "DLNA_E_INVALID_SERVICE"},
{DLNA_E_BAD_RESPONSE, "DLNA_E_BAD_RESPONSE"},
{DLNA_E_BAD_REQUEST, "DLNA_E_BAD_REQUEST"},
{DLNA_E_INVALID_ACTION, "DLNA_E_INVALID_ACTION"},
{DLNA_E_FINISH, "DLNA_E_FINISH"},
{DLNA_E_INIT_FAILED, "DLNA_E_INIT_FAILED"},
{DLNA_E_BAD_HTTPMSG, "DLNA_E_BAD_HTTPMSG"},
{DLNA_E_NETWORK_ERROR, "DLNA_E_NETWORK_ERROR"},
{DLNA_E_SOCKET_WRITE, "DLNA_E_SOCKET_WRITE"},
{DLNA_E_SOCKET_READ, "DLNA_E_SOCKET_READ"},
{DLNA_E_SOCKET_BIND, "DLNA_E_SOCKET_BIND"},
{DLNA_E_SOCKET_CONNECT, "DLNA_E_SOCKET_CONNECT"},
{DLNA_E_OUTOF_SOCKET, "DLNA_E_OUTOF_SOCKET"},
{DLNA_E_LISTEN, "DLNA_E_LISTEN"},
{DLNA_E_EVENT_PROTOCOL, "DLNA_E_EVENT_PROTOCOL"},
{DLNA_E_SUBSCRIBE_UNACCEPTED, "DLNA_E_SUBSCRIBE_UNACCEPTED"},
{DLNA_E_UNSUBSCRIBE_UNACCEPTED, "DLNA_E_UNSUBSCRIBE_UNACCEPTED"},
{DLNA_E_NOTIFY_UNACCEPTED, "DLNA_E_NOTIFY_UNACCEPTED"},
{DLNA_E_INTERNAL_ERROR, "DLNA_E_INTERNAL_ERROR"},
{DLNA_E_INVALID_ARGUMENT, "DLNA_E_INVALID_ARGUMENT"},
{DLNA_E_OUTOF_BOUNDS, "DLNA_E_OUTOF_BOUNDS"}
};

/************************************************************************
* Function : dlnaGetErrorMessage
*
* Parameters:
*	IN int rc: error code
*
* Description:
*	This functions returns the error string mapped to the error code
* Returns: const char *
*	return either the right string or "Unknown Error"
***************************************************************************/
const char *
dlnaGetErrorMessage( IN int rc )
{
    unsigned int i;

    for( i = 0; i < sizeof( ErrorMessages ) / sizeof( ErrorMessages[0] );
         i++ ) {
        if( rc == ErrorMessages[i].rc )
            return ErrorMessages[i].rcError;

    }

    return "Unknown Error";

}

/************************************************************************
* Function : dlnaResolveURL
*
* Parameters:
*	IN char * BaseURL: Base URL string
*	IN char * RelURL: relative URL string
*	OUT char * AbsURL: Absolute URL string
* Description:
*	This functions concatinates the base URL and relative URL to generate
*	the absolute URL
* Returns: int
*	return either DLNA_E_SUCCESS or appropriate error
***************************************************************************/
int
dlnaResolveURL( IN const char *BaseURL,
                IN const char *RelURL,
                OUT char *AbsURL )
{
    // There is some unnecessary allocation and
    // deallocation going on here because of the way
    // resolve_rel_url was originally written and used
    // in the future it would be nice to clean this up

    char *tempRel;

    if( RelURL == NULL )
        return DLNA_E_INVALID_PARAM;

    tempRel = NULL;

    tempRel = resolve_rel_url((char*) BaseURL, (char*) RelURL );

    if( tempRel ) {
        strcpy( AbsURL, tempRel );
        free( tempRel );
    } else {
        return DLNA_E_INVALID_URL;
    }

    return DLNA_E_SUCCESS;

}

/************************************************************************
* Function : addToAction
*
* Parameters:
*	IN int response: flag to tell if the ActionDoc is for response
*		or request
*	INOUT IXML_Document **ActionDoc: request or response document
*	IN char *ActionName: Name of the action request or response
*	IN char *ServType: Service type
*	IN char * ArgName: Name of the argument
*	IN char * ArgValue: Value of the argument
*
* Description:
*	This function adds the argument in the action request or response.
* This function creates the action request or response if it is a first
* argument else it will add the argument in the document
*
* Returns: int
*	returns DLNA_E_SUCCESS if successful else returns appropriate error
***************************************************************************/
static int
addToAction( IN int response,
             INOUT IXML_Document ** ActionDoc,
             IN const char *ActionName,
             IN const char *ServType,
             IN const char *ArgName,
             IN const char *ArgValue )
{
    char *ActBuff = NULL;
    IXML_Node *node = NULL;
    IXML_Element *Ele = NULL;
    IXML_Node *Txt = NULL;
    int rc = 0;

    if( ActionName == NULL || ServType == NULL ) {
        return DLNA_E_INVALID_PARAM;
    }

    if( *ActionDoc == NULL ) {
        ActBuff = ( char * )malloc( HEADER_LENGTH );
        if( ActBuff == NULL ) {
            return DLNA_E_OUTOF_MEMORY;
        }

        if( response ) {
            sprintf( ActBuff,
                "<u:%sResponse xmlns:u=\"%s\">\r\n</u:%sResponse>",
                ActionName, ServType, ActionName );
        } else {
            sprintf( ActBuff,
                "<u:%s xmlns:u=\"%s\">\r\n</u:%s>",
                ActionName, ServType, ActionName );
        }

        rc = ixmlParseBufferEx( ActBuff, ActionDoc );
        free( ActBuff );
        if( rc != IXML_SUCCESS ) {
            if( rc == IXML_INSUFFICIENT_MEMORY ) {
                return DLNA_E_OUTOF_MEMORY;
            } else {
                return DLNA_E_INVALID_DESC;
            }
        }
    }

    if( ArgName != NULL /*&& ArgValue != NULL */  ) {
        node = ixmlNode_getFirstChild( ( IXML_Node * ) * ActionDoc );
        Ele = ixmlDocument_createElement( *ActionDoc, ArgName );
        if( ArgValue ) {
            Txt = ixmlDocument_createTextNode( *ActionDoc, ArgValue );
            ixmlNode_appendChild( ( IXML_Node * ) Ele, Txt );
        }

        ixmlNode_appendChild( node, ( IXML_Node * ) Ele );
    }

    return DLNA_E_SUCCESS;
}

/************************************************************************
* Function : makeAction
*
* Parameters:
*	IN int response: flag to tell if the ActionDoc is for response
*		or request
*	IN char * ActionName: Name of the action request or response
*	IN char * ServType: Service type
*	IN int NumArg :Number of arguments in the action request or response
*	IN char * Arg : pointer to the first argument
*	IN va_list ArgList: Argument list
*
* Description:
*	This function creates the action request or response from the argument
* list.
* Returns: IXML_Document *
*	returns action request or response document if successful
*	else returns NULL
***************************************************************************/
static IXML_Document *
makeAction( IN int response,
            IN const char *ActionName,
            IN const char *ServType,
            IN int NumArg,
            IN const char *Arg,
            IN va_list ArgList )
{
    const char *ArgName;
    const char *ArgValue;
    char *ActBuff;
    int Idx = 0;
    IXML_Document *ActionDoc;
    IXML_Node *node;
    IXML_Element *Ele;
    IXML_Node *Txt = NULL;

    if( ActionName == NULL || ServType == NULL ) {
        return NULL;
    }

    ActBuff = ( char * )malloc( HEADER_LENGTH );
    if( ActBuff == NULL ) {
        return NULL;
    }

    if( response ) {
        sprintf( ActBuff,
            "<u:%sResponse xmlns:u=\"%s\">\r\n</u:%sResponse>",
            ActionName, ServType, ActionName );
    } else {
        sprintf( ActBuff,
            "<u:%s xmlns:u=\"%s\">\r\n</u:%s>",
            ActionName, ServType, ActionName );
    }

    if( ixmlParseBufferEx( ActBuff, &ActionDoc ) != IXML_SUCCESS ) {
        free( ActBuff );
        return NULL;
    }

    free( ActBuff );

    if( ActionDoc == NULL ) {
        return NULL;
    }

    if( NumArg > 0 ) {
        //va_start(ArgList, Arg);
        ArgName = Arg;
        for ( ; ; ) {
            ArgValue = va_arg( ArgList, const char * );

            if( ArgName != NULL ) {
                node = ixmlNode_getFirstChild( ( IXML_Node * ) ActionDoc );
                Ele = ixmlDocument_createElement( ActionDoc, ArgName );
                if( ArgValue ) {
                    Txt =
                        ixmlDocument_createTextNode( ActionDoc, ArgValue );
                    ixmlNode_appendChild( ( IXML_Node * ) Ele, Txt );
                }

                ixmlNode_appendChild( node, ( IXML_Node * ) Ele );
            }

            if (++Idx < NumArg) {
                ArgName = va_arg( ArgList, const char * );
            } else {
                break;
            }
        }
        //va_end(ArgList);
    }

    return ActionDoc;
}

/************************************************************************
* Function : dlnaMakeAction
*
* Parameters:
*	IN char * ActionName: Name of the action request or response
*	IN char * ServType: Service type
*	IN int NumArg :Number of arguments in the action request or response
*	IN char * Arg : pointer to the first argument
*	IN ... : variable argument list
*	IN va_list ArgList: Argument list
*
* Description:
*	This function creates the action request from the argument
* list. Its a wrapper function that calls makeAction function to create
* the action request.
*
* Returns: IXML_Document *
*	returns action request document if successful 
*	else returns NULL
***************************************************************************/
IXML_Document *
dlnaMakeAction( const char *ActionName,
                const char *ServType,
                int NumArg,
                const char *Arg,
                ... )
{
    va_list ArgList;
    IXML_Document *out = NULL;

    va_start( ArgList, Arg );
    out = makeAction( 0, ActionName, ServType, NumArg, Arg, ArgList );
    va_end( ArgList );

    return out;
}

/************************************************************************
* Function : dlnaMakeActionResponse
*
* Parameters:
*	IN char * ActionName: Name of the action request or response
*	IN char * ServType: Service type
*	IN int NumArg :Number of arguments in the action request or response
*	IN char * Arg : pointer to the first argument
*	IN ... : variable argument list
*	IN va_list ArgList: Argument list
*
* Description:
*	This function creates the action response from the argument
* list. Its a wrapper function that calls makeAction function to create
* the action response.
*
* Returns: IXML_Document *
*	returns action response document if successful
*	else returns NULL
***************************************************************************/
IXML_Document *
dlnaMakeActionResponse( const char *ActionName,
                        const char *ServType,
                        int NumArg,
                        const char *Arg,
                        ... )
{
    va_list ArgList;
    IXML_Document *out = NULL;

    va_start( ArgList, Arg );
    out = makeAction( 1, ActionName, ServType, NumArg, Arg, ArgList );
    va_end( ArgList );

    return out;
}

/************************************************************************
* Function : dlnaAddToActionResponse
*
* Parameters:
*	INOUT IXML_Document **ActionResponse: action response document
*	IN char * ActionName: Name of the action request or response
*	IN char * ServType: Service type
*	IN int ArgName :Name of argument to be added in the action response
*	IN char * ArgValue : value of the argument
*
* Description:
*	This function adds the argument in the action response. Its a wrapper
* function that calls addToAction function to add the argument in the
* action response.
*
* Returns: int
*	returns DLNA_E_SUCCESS if successful
*	else returns appropriate error
***************************************************************************/
int
dlnaAddToActionResponse( INOUT IXML_Document ** ActionResponse,
                         IN const char *ActionName,
                         IN const char *ServType,
                         IN const char *ArgName,
                         IN const char *ArgValue )
{
    return addToAction( 1, ActionResponse, ActionName, ServType, ArgName,
                        ArgValue );
}

/************************************************************************
* Function : dlnaAddToAction
*
* Parameters:
*	INOUT IXML_Document **ActionDoc: action request document
*	IN char * ActionName: Name of the action request or response
*	IN char * ServType: Service type
*	IN int ArgName :Name of argument to be added in the action response
*	IN char * ArgValue : value of the argument
*
* Description:
*	This function adds the argument in the action request. Its a wrapper
* function that calls addToAction function to add the argument in the
* action request.
*
* Returns: int
*	returns DLNA_E_SUCCESS if successful
*	else returns appropriate error
***************************************************************************/
int
dlnaAddToAction( IXML_Document ** ActionDoc,
                 const char *ActionName,
                 const char *ServType,
                 const char *ArgName,
                 const char *ArgValue )
{

    return addToAction( 0, ActionDoc, ActionName, ServType, ArgName,
                        ArgValue );
}

/************************************************************************
* Function : dlnaAddToPropertySet
*
* Parameters:
*	INOUT IXML_Document **PropSet: propertyset document
*	IN char *ArgName: Name of the argument
*	IN char *ArgValue: value of the argument
*
* Description:
*	This function adds the argument in the propertyset node
*
* Returns: int
*	returns DLNA_E_SUCCESS if successful else returns appropriate error
***************************************************************************/
int
dlnaAddToPropertySet( INOUT IXML_Document ** PropSet,
                      IN const char *ArgName,
                      IN const char *ArgValue )
{

    char BlankDoc[] = "<e:propertyset xmlns:e=\"urn:schemas"
        "-dlna-org:event-1-0\"></e:propertyset>";
    IXML_Node *node;
    IXML_Element *Ele;
    IXML_Element *Ele1;
    IXML_Node *Txt;
    int rc;

    if( ArgName == NULL ) {
        return DLNA_E_INVALID_PARAM;
    }

    if( *PropSet == NULL ) {
        rc = ixmlParseBufferEx( BlankDoc, PropSet );
        if( rc != IXML_SUCCESS ) {
            return DLNA_E_OUTOF_MEMORY;
        }
    }

    node = ixmlNode_getFirstChild( ( IXML_Node * ) * PropSet );

    Ele1 = ixmlDocument_createElement( *PropSet, "e:property" );
    Ele = ixmlDocument_createElement( *PropSet, ArgName );

    if( ArgValue ) {
        Txt = ixmlDocument_createTextNode( *PropSet, ArgValue );
        ixmlNode_appendChild( ( IXML_Node * ) Ele, Txt );
    }

    ixmlNode_appendChild( ( IXML_Node * ) Ele1, ( IXML_Node * ) Ele );
    ixmlNode_appendChild( node, ( IXML_Node * ) Ele1 );

    return DLNA_E_SUCCESS;
}

/************************************************************************
* Function : dlnaCreatePropertySet
*
* Parameters:
*	IN int NumArg: Number of argument that will go in the propertyset node
*	IN char * Args: argument strings
*
* Description:
*	This function creates a propertyset node and put all the input
*	parameters in the node as elements
*
* Returns: IXML_Document *
*	returns the document containing propertyset node.
***************************************************************************/
IXML_Document *
dlnaCreatePropertySet( IN int NumArg,
                       IN const char *Arg,
                       ... )
{
    va_list ArgList;
    int Idx = 0;
    char BlankDoc[] = "<e:propertyset xmlns:e=\"urn:schemas-"
        "dlna-org:event-1-0\"></e:propertyset>";
    const char *ArgName,
     *ArgValue;
    IXML_Node *node;
    IXML_Element *Ele;
    IXML_Element *Ele1;
    IXML_Node *Txt;
    IXML_Document *PropSet;

    if( ixmlParseBufferEx( BlankDoc, &PropSet ) != IXML_SUCCESS ) {
        return NULL;
    }

    if( NumArg < 1 ) {
        return NULL;
    }

    va_start( ArgList, Arg );
    ArgName = Arg;

    while( Idx++ != NumArg ) {
        ArgValue = va_arg( ArgList, const char * );

        if( ArgName != NULL /*&& ArgValue != NULL */  ) {
            node = ixmlNode_getFirstChild( ( IXML_Node * ) PropSet );
            Ele1 = ixmlDocument_createElement( PropSet, "e:property" );
            Ele = ixmlDocument_createElement( PropSet, ArgName );
            if( ArgValue ) {
                Txt = ixmlDocument_createTextNode( PropSet, ArgValue );
                ixmlNode_appendChild( ( IXML_Node * ) Ele, Txt );
            }

            ixmlNode_appendChild( ( IXML_Node * ) Ele1,
                                  ( IXML_Node * ) Ele );
            ixmlNode_appendChild( node, ( IXML_Node * ) Ele1 );
        }

        ArgName = va_arg( ArgList, const char * );

    }
    va_end( ArgList );
    return PropSet;
}

#endif // EXCLUDE_DOM == 0

