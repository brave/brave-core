// altered version
#pragma once
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


#ifndef HAPPYHTTP_H
#define HAPPYHTTP_H


#include <string>
#include <map>
#include <vector>
#include <deque>




// forward decl
struct in_addr;

namespace happyhttp
{


class Response;

// Helper Functions
void BailOnSocketError( const char* context );
struct in_addr *atoaddr( const char* address);


typedef void (*ResponseBegin_CB)( const Response* r, void* userdata );
typedef void (*ResponseData_CB)( const Response* r, void* userdata, const unsigned char* data, int numbytes );
typedef void (*ResponseComplete_CB)( const Response* r, void* userdata );


// HTTP status codes
enum {
	// 1xx informational
	CONTINUE = 100,
	SWITCHING_PROTOCOLS = 101,
	PROCESSING = 102,

	// 2xx successful
	OK = 200,
	CREATED = 201,
	ACCEPTED = 202,
	NON_AUTHORITATIVE_INFORMATION = 203,
	NO_CONTENT = 204,
	RESET_CONTENT = 205,
	PARTIAL_CONTENT = 206,
	MULTI_STATUS = 207,
	IM_USED = 226,

	// 3xx redirection
	MULTIPLE_CHOICES = 300,
	MOVED_PERMANENTLY = 301,
	FOUND = 302,
	SEE_OTHER = 303,
	NOT_MODIFIED = 304,
	USE_PROXY = 305,
	TEMPORARY_REDIRECT = 307,
	
	// 4xx client error
	BAD_REQUEST = 400,
	UNAUTHORIZED = 401,
	PAYMENT_REQUIRED = 402,
	FORBIDDEN = 403,
	NOT_FOUND = 404,
	METHOD_NOT_ALLOWED = 405,
	NOT_ACCEPTABLE = 406,
	PROXY_AUTHENTICATION_REQUIRED = 407,
	REQUEST_TIMEOUT = 408,
	CONFLICT = 409,
	GONE = 410,
	LENGTH_REQUIRED = 411,
	PRECONDITION_FAILED = 412,
	REQUEST_ENTITY_TOO_LARGE = 413,
	REQUEST_URI_TOO_LONG = 414,
	UNSUPPORTED_MEDIA_TYPE = 415,
	REQUESTED_RANGE_NOT_SATISFIABLE = 416,
	EXPECTATION_FAILED = 417,
	UNPROCESSABLE_ENTITY = 422,
	LOCKED = 423,
	FAILED_DEPENDENCY = 424,
	UPGRADE_REQUIRED = 426,

	// 5xx server error
	INTERNAL_SERVER_ERROR = 500,
	NOT_IMPLEMENTED = 501,
	BAD_GATEWAY = 502,
	SERVICE_UNAVAILABLE = 503,
	GATEWAY_TIMEOUT = 504,
	HTTP_VERSION_NOT_SUPPORTED = 505,
	INSUFFICIENT_STORAGE = 507,
	NOT_EXTENDED = 510,
};



// Exception class

class Wobbly
{
public:
	Wobbly( const char* fmt, ... );
	const char* what() const
		{ return m_Message; }
protected:
	enum { MAXLEN=256 };
	char m_Message[ MAXLEN ];
};



//-------------------------------------------------
// Connection
//
// Handles the socket connection, issuing of requests and managing
// responses.
// ------------------------------------------------

class Connection
{
	friend class Response;
public:
	// doesn't connect immediately
	Connection( const char* host, int port );
	~Connection();

	// Set up the response handling callbacks. These will be invoked during
	// calls to pump().
	// begincb		- called when the responses headers have been received
	// datacb		- called repeatedly to handle body data
	// completecb	- response is completed
	// userdata is passed as a param to all callbacks.
	void setcallbacks(
		ResponseBegin_CB begincb,
		ResponseData_CB datacb,
		ResponseComplete_CB completecb,
		void* userdata );

	// Don't need to call connect() explicitly as issuing a request will
	// call it automatically if needed.
	// But it could block (for name lookup etc), so you might prefer to
	// call it in advance.
	void connect();

	// close connection, discarding any pending requests.
	void close();

	// Update the connection (non-blocking)
	// Just keep calling this regularly to service outstanding requests.
	void pump();

	// any requests still outstanding?
	bool outstanding() const
		{ return !m_Outstanding.empty(); }

	// ---------------------------
	// high-level request interface
	// ---------------------------
	
	// method is "GET", "POST" etc...
	// url is only path part: eg  "/index.html"
	// headers is array of name/value pairs, terminated by a null-ptr
	// body & bodysize specify body data of request (eg values for a form)
	void request( const char* method, const char* url, const char* headers[]=0,
		const unsigned char* body=0, int bodysize=0 );

	// ---------------------------
	// low-level request interface
	// ---------------------------

	// begin request
	// method is "GET", "POST" etc...
	// url is only path part: eg  "/index.html"
	void putrequest( const char* method, const char* url );

	// Add a header to the request (call after putrequest() )
	void putheader( const char* header, const char* value );
	void putheader( const char* header, int numericvalue );	// alternate version

	// Finished adding headers, issue the request.
	void endheaders();

	// send body data if any.
	// To be called after endheaders()
	void send( const unsigned char* buf, int numbytes );

protected:
	// some bits of implementation exposed to Response class

	// callbacks
	ResponseBegin_CB	m_ResponseBeginCB;
	ResponseData_CB		m_ResponseDataCB;
	ResponseComplete_CB	m_ResponseCompleteCB;
	void*				m_UserData;

private:
	enum { IDLE, REQ_STARTED, REQ_SENT } m_State;
	std::string m_Host;
	int m_Port;
	int m_Sock;
	std::vector< std::string > m_Buffer;	// lines of request

	std::deque< Response* > m_Outstanding;	// responses for outstanding requests
};






//-------------------------------------------------
// Response
//
// Handles parsing of response data.
// ------------------------------------------------


class Response
{
	friend class Connection;
public:

	// retrieve a header (returns 0 if not present)
	const char* getheader( const char* name ) const;

	bool completed() const
		{ return m_State == COMPLETE; }


	// get the HTTP status code
	int getstatus() const;

	// get the HTTP response reason string
	const char* getreason() const;

	// true if connection is expected to close after this response.
	bool willclose() const
		{ return m_WillClose; }
protected:
	// interface used by Connection

	// only Connection creates Responses.
	Response( const char* method, Connection& conn );

  ~Response();

	// pump some data in for processing.
	// Returns the number of bytes used.
	// Will always return 0 when response is complete.
	int pump( const unsigned char* data, int datasize );

	// tell response that connection has closed
	void notifyconnectionclosed();

private:
	enum {
		STATUSLINE,		// start here. status line is first line of response.
		HEADERS,		// reading in header lines
		BODY,			// waiting for some body data (all or a chunk)
		CHUNKLEN,		// expecting a chunk length indicator (in hex)
		CHUNKEND,		// got the chunk, now expecting a trailing blank line
		TRAILERS,		// reading trailers after body.
		COMPLETE,		// response is complete!
	} m_State;

	Connection& m_Connection;	// to access callback ptrs
	std::string m_Method;		// req method: "GET", "POST" etc...

	// status line
	std::string	m_VersionString;	// HTTP-Version
	int	m_Version;			// 10: HTTP/1.0    11: HTTP/1.x (where x>=1)
	int m_Status;			// Status-Code
	std::string m_Reason;	// Reason-Phrase

	// header/value pairs
	std::map<std::string,std::string> m_Headers;

	int		m_BytesRead;		// body bytes read so far
	bool	m_Chunked;			// response is chunked?
	int		m_ChunkLeft;		// bytes left in current chunk
	int		m_Length;			// -1 if unknown
	bool	m_WillClose;		// connection will close at response end?

	std::string m_LineBuf;		// line accumulation for states that want it
	std::string m_HeaderAccum;	// accumulation buffer for headers


	void FlushHeader();
	void ProcessStatusLine( std::string const& line );
	void ProcessHeaderLine( std::string const& line );
	void ProcessTrailerLine( std::string const& line );
	void ProcessChunkLenLine( std::string const& line );

	int ProcessDataChunked( const unsigned char* data, int count );
	int ProcessDataNonChunked( const unsigned char* data, int count );

	void BeginBody();
	bool CheckClose();
	void Finish();
};



}	// end namespace happyhttp


#endif // HAPPYHTTP_H


