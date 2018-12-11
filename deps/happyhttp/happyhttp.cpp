// altered version
/*
 * HappyHTTP - a simple HTTP library
 * Version 0.1
 * 
 * Copyright (c) 2006 Ben Campbell
 *
 * This software is provided 'as-is', without any express or implied
 * warranty. In no event will the authors be held liable for any damages
 * arising from the use of this software.
 * 
 * Permission is granted to anyone to use this software for any purpose,
 * including commercial applications, and to alter it and redistribute it
 * freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not
 * claim that you wrote the original software. If you use this software in a
 * product, an acknowledgment in the product documentation would be
 * appreciated but is not required.
 *
 * 2. Altered source versions must be plainly marked as such, and must not
 * be misrepresented as being the original software.
 * 
 * 3. This notice may not be removed or altered from any source distribution.
 *
 */


#include "happyhttp.h"

#ifndef WIN32
//	#include <sys/types.h>
	#include <sys/socket.h>
	#include <netinet/in.h>
	#include <arpa/inet.h>
	#include <netdb.h>	// for gethostbyname()
	#include <errno.h>
#endif

#ifdef WIN32
	#include <winsock2.h>
	#define vsnprintf _vsnprintf
#endif

#include <cstdio>
#include <cstring>
#include <cstdarg>
#include <assert.h>

#include <string>
#include <vector>
#include <string>
#include <algorithm>
#include <iostream>

// kevin add to get this thing to compile
#include <unistd.h>
#include <sys/select.h>


using namespace std;


namespace happyhttp
{

#ifdef WIN32
const char* GetWinsockErrorString( int err );
#endif


//---------------------------------------------------------------------
// Helper functions
//---------------------------------------------------------------------



void BailOnSocketError( const char* context )
{
#ifdef WIN32

	int e = WSAGetLastError();
	const char* msg = GetWinsockErrorString( e );
#else
	const char* msg = strerror( errno );
#endif
	// throw Wobbly( "%s: %s", context, msg );
  if (msg) {
  }
}


#ifdef WIN32

const char* GetWinsockErrorString( int err )
{
	switch( err)
	{
	case 0:					return "No error";
    case WSAEINTR:			return "Interrupted system call";
    case WSAEBADF:			return "Bad file number";
    case WSAEACCES:			return "Permission denied";
    case WSAEFAULT:			return "Bad address";
    case WSAEINVAL:			return "Invalid argument";
    case WSAEMFILE:			return "Too many open sockets";
    case WSAEWOULDBLOCK:	return "Operation would block";
    case WSAEINPROGRESS:	return "Operation now in progress";
    case WSAEALREADY:		return "Operation already in progress";
    case WSAENOTSOCK:		return "Socket operation on non-socket";
    case WSAEDESTADDRREQ:	return "Destination address required";
    case WSAEMSGSIZE:		return "Message too long";
    case WSAEPROTOTYPE:		return "Protocol wrong type for socket";
    case WSAENOPROTOOPT:	return "Bad protocol option";
	case WSAEPROTONOSUPPORT:	return "Protocol not supported";
	case WSAESOCKTNOSUPPORT:	return "Socket type not supported";
    case WSAEOPNOTSUPP:		return "Operation not supported on socket";
    case WSAEPFNOSUPPORT:	return "Protocol family not supported";
    case WSAEAFNOSUPPORT:	return "Address family not supported";
    case WSAEADDRINUSE:		return "Address already in use";
    case WSAEADDRNOTAVAIL:	return "Can't assign requested address";
    case WSAENETDOWN:		return "Network is down";
    case WSAENETUNREACH:	return "Network is unreachable";
    case WSAENETRESET:		return "Net connection reset";
    case WSAECONNABORTED:	return "Software caused connection abort";
    case WSAECONNRESET:		return "Connection reset by peer";
    case WSAENOBUFS:		return "No buffer space available";
    case WSAEISCONN:		return "Socket is already connected";
    case WSAENOTCONN:		return "Socket is not connected";
    case WSAESHUTDOWN:		return "Can't send after socket shutdown";
    case WSAETOOMANYREFS:	return "Too many references, can't splice";
    case WSAETIMEDOUT:		return "Connection timed out";
    case WSAECONNREFUSED:	return "Connection refused";
    case WSAELOOP:			return "Too many levels of symbolic links";
    case WSAENAMETOOLONG:	return "File name too long";
    case WSAEHOSTDOWN:		return "Host is down";
    case WSAEHOSTUNREACH:	return "No route to host";
    case WSAENOTEMPTY:		return "Directory not empty";
    case WSAEPROCLIM:		return "Too many processes";
    case WSAEUSERS:			return "Too many users";
    case WSAEDQUOT:			return "Disc quota exceeded";
    case WSAESTALE:			return "Stale NFS file handle";
    case WSAEREMOTE:		return "Too many levels of remote in path";
	case WSASYSNOTREADY:	return "Network system is unavailable";
	case WSAVERNOTSUPPORTED:	return "Winsock version out of range";
    case WSANOTINITIALISED:	return "WSAStartup not yet called";
    case WSAEDISCON:		return "Graceful shutdown in progress";
    case WSAHOST_NOT_FOUND:	return "Host not found";
    case WSANO_DATA:		return "No host data of that type was found";
	}

	return "unknown";
};

#endif // WIN32


// return true if socket has data waiting to be read
bool datawaiting( int sock )
{
	fd_set fds;
	FD_ZERO( &fds );
	FD_SET( sock, &fds );

	struct timeval tv;
	tv.tv_sec = 0;
	tv.tv_usec = 0;

	int r = select( sock+1, &fds, NULL, NULL, &tv);
	if (r < 0)
		BailOnSocketError( "select" );

	if( FD_ISSET( sock, &fds ) )
		return true;
	else
		return false;
}


// Try to work out address from string
// returns 0 if bad
struct in_addr *atoaddr( const char* address)
{
	struct hostent *host;
	static struct in_addr saddr;

	// First try nnn.nnn.nnn.nnn form
	saddr.s_addr = inet_addr(address);
	if (saddr.s_addr != (unsigned int)-1)
		return &saddr;

	host = gethostbyname(address);
	if( host )
		return (struct in_addr *) *host->h_addr_list;

	return 0;
}







//---------------------------------------------------------------------
//
// Exception class
//
//---------------------------------------------------------------------


Wobbly::Wobbly( const char* fmt, ... )
{
	va_list ap;
	va_start( ap,fmt);
	int n = vsnprintf( m_Message, MAXLEN, fmt, ap );
	va_end( ap );
	if(n==MAXLEN)
		m_Message[MAXLEN-1] = '\0';
}








//---------------------------------------------------------------------
//
// Connection
//
//---------------------------------------------------------------------
Connection::Connection( const char* host, int port ) :
	m_ResponseBeginCB(0),
	m_ResponseDataCB(0),
	m_ResponseCompleteCB(0),
	m_UserData(0),
	m_State( IDLE ),
	m_Host( host ),
	m_Port( port ),
	m_Sock(-1)
{
}


void Connection::setcallbacks(
	ResponseBegin_CB begincb,
	ResponseData_CB datacb,
	ResponseComplete_CB completecb,
	void* userdata )
{
	m_ResponseBeginCB = begincb;
	m_ResponseDataCB = datacb;
	m_ResponseCompleteCB = completecb;
	m_UserData = userdata;
}


void Connection::connect()
{
	in_addr* addr = atoaddr( m_Host.c_str() );
	if( !addr )
    std::cerr << "Invalid network address\n";
		//throw Wobbly( "Invalid network address" );

	sockaddr_in address;
	memset( (char*)&address, 0, sizeof(address) );
	address.sin_family = AF_INET;
	address.sin_port = htons( m_Port );
	address.sin_addr.s_addr = addr->s_addr;

	m_Sock = socket( AF_INET, SOCK_STREAM, 0 );
	if( m_Sock < 0 )
		BailOnSocketError( "socket()" );

//	printf("Connecting to %s on port %d.\n",inet_ntoa(*addr), port);

	if( ::connect( m_Sock, (sockaddr const*)&address, sizeof(address) ) < 0 )
		BailOnSocketError( "connect()" );
}


void Connection::close()
{
#ifdef WIN32
	if( m_Sock >= 0 )
		::closesocket( m_Sock );
#else
	if( m_Sock >= 0 )
		::close( m_Sock );
#endif
	m_Sock = -1;

	// discard any incomplete responses
	while( !m_Outstanding.empty() )
	{
		delete m_Outstanding.front();
		m_Outstanding.pop_front();
	}
}


Connection::~Connection()
{
	close();
}

void Connection::request( const char* method,
	const char* url,
	const char* headers[],
	const unsigned char* body,
	int bodysize )
{

	bool gotcontentlength = false;	// already in headers?

	// check headers for content-length
	// TODO: check for "Host" and "Accept-Encoding" too
	// and avoid adding them ourselves in putrequest()
	if( headers )
	{
		const char** h = headers;
		while( *h )
		{
			const char* name = *h++;
			const char* value = *h++;
			assert( value != 0 );	// name with no value!

			if( 0==strcasecmp( name, "content-length" ) )
				gotcontentlength = true;
		}
	}

	putrequest( method, url );

	if( body && !gotcontentlength )
		putheader( "Content-Length", bodysize );

	if( headers )
	{
		const char** h = headers;
		while( *h )
		{
			const char* name = *h++;
			const char* value = *h++;
			putheader( name, value );
		}
	}
	endheaders();

	if( body )
		send( body, bodysize );

}




void Connection::putrequest( const char* method, const char* url )
{
	if( m_State != IDLE )
    std::cerr << "Request already issued\n";
		// throw Wobbly( "Request already issued" );

	m_State = REQ_STARTED;

	char req[ 512*100 ];
	sprintf( req, "%s %s HTTP/1.1", method, url );
	m_Buffer.push_back( req );

	putheader( "Host", m_Host.c_str() );	// required for HTTP1.1

	// don't want any fancy encodings please
	putheader("Accept-Encoding", "identity");

	// Push a new response onto the queue
	Response *r = new Response( method, *this );
	m_Outstanding.push_back( r );
}


void Connection::putheader( const char* header, const char* value )
{
	if( m_State != REQ_STARTED )
    std::cerr << "putheader failed \n";
		// throw Wobbly( "putheader() failed" );
	m_Buffer.push_back( string(header) + ": " + string( value ) );
}

void Connection::putheader( const char* header, int numericvalue )
{
	char buf[32];
	sprintf( buf, "%d", numericvalue );
	putheader( header, buf );
}

void Connection::endheaders()
{
	if( m_State != REQ_STARTED )
    std::cerr << "Cannot send header\n";
		// throw Wobbly( "Cannot send header" );
	m_State = IDLE;

	m_Buffer.push_back( "" );

	string msg;
	vector< string>::const_iterator it;
	for( it = m_Buffer.begin(); it != m_Buffer.end(); ++it )
		msg += (*it) + "\r\n";

	m_Buffer.clear();

//	printf( "%s", msg.c_str() );
	send( (const unsigned char*)msg.c_str(), msg.size() );
}



void Connection::send( const unsigned char* buf, int numbytes )
{
//	fwrite( buf, 1,numbytes, stdout );
	
	if( m_Sock < 0 )
		connect();

	while( numbytes > 0 )
	{
#ifdef WIN32
		int n = ::send( m_Sock, (const char*)buf, numbytes, 0 );
#else
		int n = ::send( m_Sock, buf, numbytes, 0 );
#endif
		if( n<0 )
			BailOnSocketError( "send()" );
		numbytes -= n;
		buf += n;
	}
}


void Connection::pump()
{
	if( m_Outstanding.empty() )
		return;		// no requests outstanding

	assert( m_Sock >0 );	// outstanding requests but no connection!

	if( !datawaiting( m_Sock ) )
		return;				// recv will block

	unsigned char buf[ 2048 ];
	int a = recv( m_Sock, (char*)buf, sizeof(buf), 0 );
	if( a<0 )
		BailOnSocketError( "recv()" );

	if( a== 0 )
	{
		// connection has closed

		Response* r = m_Outstanding.front();
		r->notifyconnectionclosed();
		assert( r->completed() );
		delete r;
		m_Outstanding.pop_front();

		// any outstanding requests will be discarded
		close();
	}
	else
	{
		int used = 0;
		while( used < a && !m_Outstanding.empty() )
		{

			Response* r = m_Outstanding.front();
			int u = r->pump( &buf[used], a-used );

			// delete response once completed
			if( r->completed() )
			{
				delete r;
				m_Outstanding.pop_front();
			}
			used += u;
		}

		// NOTE: will lose bytes if response queue goes empty
		// (but server shouldn't be sending anything if we don't have
		// anything outstanding anyway)
		assert( used == a );	// all bytes should be used up by here.
	}
}






//---------------------------------------------------------------------
//
// Response
//
//---------------------------------------------------------------------


Response::Response( const char* method, Connection& conn ) :
	m_State( STATUSLINE ),
	m_Connection( conn ),
	m_Method( method ),
	m_Version( 0 ),
	m_Status(0),
	m_BytesRead(0),
	m_Chunked(false),
	m_ChunkLeft(0),
	m_Length(-1),
	m_WillClose(false)
{
}

Response::~Response() {

}

const char* Response::getheader( const char* name ) const
{
	std::string lname( name );
//	std::transform( lname.begin(), lname.end(), lname.begin(), tolower );

	std::map< std::string, std::string >::const_iterator it = m_Headers.find( lname );
	if( it == m_Headers.end() )
		return 0;
	else
		return it->second.c_str();
}


int Response::getstatus() const
{
	// only valid once we've got the statusline
	assert( m_State != STATUSLINE );
	return m_Status;
}


const char* Response::getreason() const
{
	// only valid once we've got the statusline
	assert( m_State != STATUSLINE );
	return m_Reason.c_str();
}



// Connection has closed
void Response::notifyconnectionclosed()
{
	if( m_State == COMPLETE )
		return;

	// eof can be valid...
	if( m_State == BODY &&
		!m_Chunked &&
		m_Length == -1 )
	{
		Finish();	// we're all done!
	}
	else
	{
    std::cerr << "Connection closed unexpectedly\n";
		// throw Wobbly( "Connection closed unexpectedly" );
	}
}



int Response::pump( const unsigned char* data, int datasize )
{
	assert( datasize != 0 );
	int count = datasize;

	while( count > 0 && m_State != COMPLETE )
	{
		if( m_State == STATUSLINE ||
			m_State == HEADERS ||
			m_State == TRAILERS ||
			m_State == CHUNKLEN ||
			m_State == CHUNKEND )
		{
			// we want to accumulate a line
			while( count > 0 )
			{
				char c = (char)*data++;
				--count;
				if( c == '\n' )
				{
					// now got a whole line!
					switch( m_State )
					{
						case STATUSLINE:
							ProcessStatusLine( m_LineBuf );
							break;
						case HEADERS:
							ProcessHeaderLine( m_LineBuf );
							break;
						case TRAILERS:
							ProcessTrailerLine( m_LineBuf );
							break;
						case CHUNKLEN:
							ProcessChunkLenLine( m_LineBuf );
							break;
						case CHUNKEND:
							// just soak up the crlf after body and go to next state
							assert( m_Chunked == true );
							m_State = CHUNKLEN;
							break;
						default:
							break;
					}
					m_LineBuf.clear();
					break;		// break out of line accumulation!
				}
				else
				{
					if( c != '\r' )		// just ignore CR
						m_LineBuf += c;
				}
			}
		}
		else if( m_State == BODY )
		{
			int bytesused = 0;
			if( m_Chunked )
				bytesused = ProcessDataChunked( data, count );
			else
				bytesused = ProcessDataNonChunked( data, count );
			data += bytesused;
			count -= bytesused;
		}
	}

	// return number of bytes used
	return datasize - count;
}



void Response::ProcessChunkLenLine( std::string const& line )
{
	// chunklen in hex at beginning of line
	m_ChunkLeft = strtol( line.c_str(), NULL, 16 );
	
	if( m_ChunkLeft == 0 )
	{
		// got the whole body, now check for trailing headers
		m_State = TRAILERS;
		m_HeaderAccum.clear();
	}
	else
	{
		m_State = BODY;
	}
}


// handle some body data in chunked mode
// returns number of bytes used.
int Response::ProcessDataChunked( const unsigned char* data, int count )
{
	assert( m_Chunked );

	int n = count;
	if( n>m_ChunkLeft )
		n = m_ChunkLeft;

	// invoke callback to pass out the data
	if( m_Connection.m_ResponseDataCB )
		(m_Connection.m_ResponseDataCB)( this, m_Connection.m_UserData, data, n );

	m_BytesRead += n;

	m_ChunkLeft -= n;
	assert( m_ChunkLeft >= 0);
	if( m_ChunkLeft == 0 )
	{
		// chunk completed! now soak up the trailing CRLF before next chunk
		m_State = CHUNKEND;
	}
	return n;
}

// handle some body data in non-chunked mode.
// returns number of bytes used.
int Response::ProcessDataNonChunked( const unsigned char* data, int count )
{
	int n = count;
	if( m_Length != -1 )
	{
		// we know how many bytes to expect
		int remaining = m_Length - m_BytesRead;
		if( n > remaining )
			n = remaining;
	}

	// invoke callback to pass out the data
	if( m_Connection.m_ResponseDataCB )
		(m_Connection.m_ResponseDataCB)( this, m_Connection.m_UserData, data, n );

	m_BytesRead += n;

	// Finish if we know we're done. Else we're waiting for connection close.
	if( m_Length != -1 && m_BytesRead == m_Length )
		Finish();

	return n;
}


void Response::Finish()
{
	m_State = COMPLETE;

	// invoke the callback
	if( m_Connection.m_ResponseCompleteCB )
		(m_Connection.m_ResponseCompleteCB)( this, m_Connection.m_UserData );
}


void Response::ProcessStatusLine( std::string const& line )
{
	const char* p = line.c_str();

	// skip any leading space
	while( *p && *p == ' ' )
		++p;

	// get version
	while( *p && *p != ' ' )
		m_VersionString += *p++;
	while( *p && *p == ' ' )
		++p;

	// get status code
	std::string status;
	while( *p && *p != ' ' )
		status += *p++;
	while( *p && *p == ' ' )
		++p;

	// rest of line is reason
	while( *p )
		m_Reason += *p++;

	m_Status = atoi( status.c_str() );
	if( m_Status < 100 || m_Status > 999 )
    std::cerr << "Bad status line\n";
		// throw Wobbly( "BadStatusLine (%s)", line.c_str() );

/*
	printf( "version: '%s'\n", m_VersionString.c_str() );
	printf( "status: '%d'\n", m_Status );
	printf( "reason: '%s'\n", m_Reason.c_str() );
*/

	if( m_VersionString == "HTTP:/1.0" )
		m_Version = 10;
	else if( 0==m_VersionString.compare( 0,7,"HTTP/1." ) )
		m_Version = 11;
	else
    std::cerr << "unknown protocol \n";
		// throw Wobbly( "UnknownProtocol (%s)", m_VersionString.c_str() );
	// TODO: support for HTTP/0.9

	
	// OK, now we expect headers!
	m_State = HEADERS;
	m_HeaderAccum.clear();
}


// process accumulated header data
void Response::FlushHeader()
{
	if( m_HeaderAccum.empty() )
		return;	// no flushing required

	const char* p = m_HeaderAccum.c_str();

	std::string header;
	std::string value;
	while( *p && *p != ':' )
		header += tolower( *p++ );

	// skip ':'
	if( *p )
		++p;

	// skip space
	while( *p && (*p ==' ' || *p=='\t') )
		++p;

	value = p; // rest of line is value

	m_Headers[ header ] = value;
//	printf("header: ['%s': '%s']\n", header.c_str(), value.c_str() );	

	m_HeaderAccum.clear();
}


void Response::ProcessHeaderLine( std::string const& line )
{
	const char* p = line.c_str();
	if( line.empty() )
	{
		FlushHeader();
		// end of headers

		// HTTP code 100 handling (we ignore 'em)
		if( m_Status == CONTINUE )
			m_State = STATUSLINE;	// reset parsing, expect new status line
		else
			BeginBody();			// start on body now!
		return;
	}

	if( isspace(*p) )
	{
		// it's a continuation line - just add it to previous data
		++p;
		while( *p && isspace( *p ) )
			++p;

		m_HeaderAccum += ' ';
		m_HeaderAccum += p;
	}
	else
	{
		// begin a new header
		FlushHeader();
		m_HeaderAccum = p;
	}
}


void Response::ProcessTrailerLine( std::string const& line )
{
	// TODO: handle trailers?
	// (python httplib doesn't seem to!)
	if( line.empty() )
		Finish();

	// just ignore all the trailers...
}



// OK, we've now got all the headers read in, so we're ready to start
// on the body. But we need to see what info we can glean from the headers
// first...
void Response::BeginBody()
{

	m_Chunked = false;
	m_Length = -1;	// unknown
	m_WillClose = false;

	// using chunked encoding?
	const char* trenc = getheader( "transfer-encoding" );
	if( trenc && 0==strcasecmp( trenc, "chunked") )
	{
		m_Chunked = true;
		m_ChunkLeft = -1;	// unknown
	}

	m_WillClose = CheckClose();

	// length supplied?
	const char* contentlen = getheader( "content-length" );
	if( contentlen && !m_Chunked )
	{
		m_Length = atoi( contentlen );
	}

	// check for various cases where we expect zero-length body
	if( m_Status == NO_CONTENT ||
		m_Status == NOT_MODIFIED ||
		( m_Status >= 100 && m_Status < 200 ) ||		// 1xx codes have no body
		m_Method == "HEAD" )
	{
		m_Length = 0;
	}


	// if we're not using chunked mode, and no length has been specified,
	// assume connection will close at end.
	if( !m_WillClose && !m_Chunked && m_Length == -1 )
		m_WillClose = true;



	// Invoke the user callback, if any
	if( m_Connection.m_ResponseBeginCB )
		(m_Connection.m_ResponseBeginCB)( this, m_Connection.m_UserData );

/*
	printf("---------BeginBody()--------\n");
	printf("Length: %d\n", m_Length );
	printf("WillClose: %d\n", (int)m_WillClose );
	printf("Chunked: %d\n", (int)m_Chunked );
	printf("ChunkLeft: %d\n", (int)m_ChunkLeft );
	printf("----------------------------\n");
*/
	// now start reading body data!
	if( m_Chunked )
		m_State = CHUNKLEN;
	else
		m_State = BODY;
}


// return true if we think server will automatically close connection
bool Response::CheckClose()
{
	if( m_Version == 11 )
	{
		// HTTP1.1
		// the connection stays open unless "connection: close" is specified.
		const char* conn = getheader( "connection" );
		if( conn && 0==strcasecmp( conn, "close" ) )
			return true;
		else
			return false;
	}

	// Older HTTP
	// keep-alive header indicates persistant connection 
	if( getheader( "keep-alive" ) )
		return false;

	// TODO: some special case handling for Akamai and netscape maybe?
	// (see _check_close() in python httplib.py for details)

	return true;
}



}	// end namespace happyhttp


