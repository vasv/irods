/* -*- mode: c++; fill-column: 132; c-basic-offset: 4; indent-tabs-mode: nil -*- */

/*** Copyright (c), The Regents of the University of California            ***
 *** For more information please refer to files in the COPYRIGHT directory ***/
/* This is script-generated code (for the most part).  */ 
/* See fileGet.h for a description of this API call.*/

#include "fileGet.h"
#include "miscServerFunct.h"

// =-=-=-=-=-=-=-
// eirods includes
#include "eirods_log.h"
#include "eirods_file_object.h"
#include "eirods_resource_backport.h"


/* rsFileGet - Get the content of a small file into a single buffer
 * in fileGetOutBBuf->buf.
 * Return value - int - number of bytes read.
 */

int
rsFileGet (rsComm_t *rsComm, fileOpenInp_t *fileGetInp, 
           bytesBuf_t *fileGetOutBBuf)
{
    rodsServerHost_t *rodsServerHost;
    int remoteFlag;
    int status;

    //remoteFlag = resolveHost (&fileGetInp->addr, &rodsServerHost);
    eirods::error ret = eirods::get_host_for_hier_string( fileGetInp->resc_hier_, remoteFlag, rodsServerHost );
    if( !ret.ok() ) {
        eirods::log( PASSMSG( "failed in call to eirods::get_host_for_hier_string", ret ) );
        return -1;
    }
    if (remoteFlag == LOCAL_HOST) {
        status = _rsFileGet (rsComm, fileGetInp, fileGetOutBBuf);
    } else if (remoteFlag == REMOTE_HOST) {
        status = remoteFileGet (rsComm, fileGetInp, fileGetOutBBuf, 
                                rodsServerHost);
    } else {
        if (remoteFlag < 0) {
            return (remoteFlag);
        } else {
            rodsLog (LOG_NOTICE,
                     "rsFileGet: resolveHost returned unrecognized value %d",
                     remoteFlag);
            return (SYS_UNRECOGNIZED_REMOTE_FLAG);
        }
    }

    return (status);
}

int
remoteFileGet (rsComm_t *rsComm, fileOpenInp_t *fileGetInp,
               bytesBuf_t *fileGetOutBBuf, rodsServerHost_t *rodsServerHost)
{    
    int status;

    if (rodsServerHost == NULL) {
        rodsLog (LOG_NOTICE,
                 "remoteFileGet: Invalid rodsServerHost");
        return SYS_INVALID_SERVER_HOST;
    }

    if ((status = svrToSvrConnect (rsComm, rodsServerHost)) < 0) {
        return status;
    }



    status = rcFileGet (rodsServerHost->conn, fileGetInp, fileGetOutBBuf);

    if (status < 0) { 
        rodsLog (LOG_NOTICE,
                 "remoteFileGet: rcFileGet failed for %s",
                 fileGetInp->fileName);
    }

    return status;
}

int _rsFileGet (rsComm_t *rsComm, fileOpenInp_t *fileGetInp, bytesBuf_t *fileGetOutBBuf ) {

    int fd;
    int len;

    len = fileGetInp->dataSize;
    if (len <= 0)
        return (0);
    fd = _rsFileOpen (rsComm, fileGetInp);

    if (fd < 0) {
        rodsLog (LOG_NOTICE,
                 "_rsFileGet: fileGet for %s, status = %d",
                 fileGetInp->fileName, fd);
        return (fd);
    }

    if (fileGetOutBBuf->buf == NULL) {
        fileGetOutBBuf->buf = malloc (len);
    }

    if(fileGetInp->objPath[0] == '\0') {
        std::stringstream msg;
        msg << __FUNCTION__;
        msg << " - Empty logical path.";
        eirods::log(LOG_ERROR, msg.str());
        return -1;
    }
    
    eirods::file_object file_obj( rsComm, fileGetInp->objPath, fileGetInp->fileName, fileGetInp->resc_hier_, fd, fileGetInp->mode, fileGetInp->flags  );
    eirods::error read_err = fileRead( rsComm,
                                       file_obj,
                                       fileGetOutBBuf->buf, 
                                       len );
    int bytes_read = read_err.code();
    if ( bytes_read != len ) {
        if ( bytes_read >= 0) {
        
            fileGetOutBBuf->len = bytes_read;

        } else {
            std::stringstream msg;
            msg << "fileRead failed for [";
            msg << fileGetInp->fileName;
            msg << "]";
            eirods::error ret_err = PASSMSG( msg.str(), read_err );
            eirods::log( ret_err );
        }
    } else {
        fileGetOutBBuf->len = bytes_read;
    }

    // =-=-=-=-=-=-=-
    // call resource plugin close 
    eirods::error close_err = fileClose( rsComm,
                                         file_obj );
    if( !close_err.ok() ) {
        eirods::error err = PASSMSG( "error on close", close_err );
        eirods::log( err );
    }

    return (bytes_read);

} // _rsFileGet 







