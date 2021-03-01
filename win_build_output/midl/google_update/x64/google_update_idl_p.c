

/* this ALWAYS GENERATED file contains the proxy stub code */


 /* File created by MIDL compiler version 8.xx.xxxx */
/* at a redacted point in time
 */
/* Compiler settings for ../../brave/chromium_src/google_update/google_update_idl.idl:
    Oicf, W1, Zp8, env=Win64 (32b run), target_arch=AMD64 8.xx.xxxx 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#if defined(_M_AMD64)


#pragma warning( disable: 4049 )  /* more than 64k source lines */
#if _MSC_VER >= 1200
#pragma warning(push)
#endif

#pragma warning( disable: 4211 )  /* redefine extern to static */
#pragma warning( disable: 4232 )  /* dllimport identity*/
#pragma warning( disable: 4024 )  /* array to pointer mapping*/
#pragma warning( disable: 4152 )  /* function/data pointer conversion in expression */

#define USE_STUBLESS_PROXY


/* verify that the <rpcproxy.h> version is high enough to compile this file*/
#ifndef __REDQ_RPCPROXY_H_VERSION__
#define __REQUIRED_RPCPROXY_H_VERSION__ 475
#endif


#include "rpcproxy.h"
#ifndef __RPCPROXY_H_VERSION__
#error this stub requires an updated version of <rpcproxy.h>
#endif /* __RPCPROXY_H_VERSION__ */


#include "google_update_idl.h"

#define TYPE_FORMAT_STRING_SIZE   1117                              
#define PROC_FORMAT_STRING_SIZE   5237                              
#define EXPR_FORMAT_STRING_SIZE   1                                 
#define TRANSMIT_AS_TABLE_SIZE    0            
#define WIRE_MARSHAL_TABLE_SIZE   2            

typedef struct _google_update_idl_MIDL_TYPE_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ TYPE_FORMAT_STRING_SIZE ];
    } google_update_idl_MIDL_TYPE_FORMAT_STRING;

typedef struct _google_update_idl_MIDL_PROC_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ PROC_FORMAT_STRING_SIZE ];
    } google_update_idl_MIDL_PROC_FORMAT_STRING;

typedef struct _google_update_idl_MIDL_EXPR_FORMAT_STRING
    {
    long          Pad;
    unsigned char  Format[ EXPR_FORMAT_STRING_SIZE ];
    } google_update_idl_MIDL_EXPR_FORMAT_STRING;


static const RPC_SYNTAX_IDENTIFIER  _RpcTransferSyntax = 
{{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}};


extern const google_update_idl_MIDL_TYPE_FORMAT_STRING google_update_idl__MIDL_TypeFormatString;
extern const google_update_idl_MIDL_PROC_FORMAT_STRING google_update_idl__MIDL_ProcFormatString;
extern const google_update_idl_MIDL_EXPR_FORMAT_STRING google_update_idl__MIDL_ExprFormatString;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IGoogleUpdate3_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IGoogleUpdate3_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IAppBundle_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IAppBundle_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IApp_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IApp_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IApp2_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IApp2_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IAppCommand_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IAppCommand_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IAppCommand2_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IAppCommand2_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IAppVersion_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IAppVersion_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IPackage_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IPackage_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICurrentState_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICurrentState_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IRegistrationUpdateHook_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IRegistrationUpdateHook_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICredentialDialog_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICredentialDialog_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IGoogleUpdate3Web_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IGoogleUpdate3Web_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IGoogleUpdate3WebSecurity_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IGoogleUpdate3WebSecurity_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IAppBundleWeb_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IAppBundleWeb_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IAppWeb_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IAppWeb_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IAppCommandWeb_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IAppCommandWeb_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IAppVersionWeb_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IAppVersionWeb_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICoCreateAsyncStatus_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICoCreateAsyncStatus_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO ICoCreateAsync_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO ICoCreateAsync_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IBrowserHttpRequest2_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IBrowserHttpRequest2_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IProcessLauncher_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IProcessLauncher_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IProcessLauncher2_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IProcessLauncher2_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IOneClickProcessLauncher_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IOneClickProcessLauncher_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IProgressWndEvents_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IProgressWndEvents_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IJobObserver_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IJobObserver_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IJobObserver2_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IJobObserver2_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IGoogleUpdate_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IGoogleUpdate_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IGoogleUpdateCore_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IGoogleUpdateCore_ProxyInfo;


extern const USER_MARSHAL_ROUTINE_QUADRUPLE UserMarshalRoutines[ WIRE_MARSHAL_TABLE_SIZE ];

#if !defined(__RPC_WIN64__)
#error  Invalid build platform for this stub.
#endif

static const google_update_idl_MIDL_PROC_FORMAT_STRING google_update_idl__MIDL_ProcFormatString =
    {
        0,
        {

	/* Procedure get_status */


	/* Procedure get_stateValue */


	/* Procedure get_Count */

			0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/*  2 */	NdrFcLong( 0x0 ),	/* 0 */
/*  6 */	NdrFcShort( 0x7 ),	/* 7 */
/*  8 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 10 */	NdrFcShort( 0x0 ),	/* 0 */
/* 12 */	NdrFcShort( 0x24 ),	/* 36 */
/* 14 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 16 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 18 */	NdrFcShort( 0x0 ),	/* 0 */
/* 20 */	NdrFcShort( 0x0 ),	/* 0 */
/* 22 */	NdrFcShort( 0x0 ),	/* 0 */
/* 24 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IAppCommandWeb0000 */


	/* Parameter __MIDL__ICurrentState0000 */


	/* Parameter count */

/* 26 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 28 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 30 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */


	/* Return value */


	/* Return value */

/* 32 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 34 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 36 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_Item */

/* 38 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 40 */	NdrFcLong( 0x0 ),	/* 0 */
/* 44 */	NdrFcShort( 0x8 ),	/* 8 */
/* 46 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 48 */	NdrFcShort( 0x8 ),	/* 8 */
/* 50 */	NdrFcShort( 0x8 ),	/* 8 */
/* 52 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x3,		/* 3 */
/* 54 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 56 */	NdrFcShort( 0x0 ),	/* 0 */
/* 58 */	NdrFcShort( 0x0 ),	/* 0 */
/* 60 */	NdrFcShort( 0x0 ),	/* 0 */
/* 62 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter index */

/* 64 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 66 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 68 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter bundle */

/* 70 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 72 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 74 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Return value */

/* 76 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 78 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 80 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_createdInstance */


	/* Procedure get_nextVersionWeb */


	/* Procedure createAppBundle */

/* 82 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 84 */	NdrFcLong( 0x0 ),	/* 0 */
/* 88 */	NdrFcShort( 0x9 ),	/* 9 */
/* 90 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 92 */	NdrFcShort( 0x0 ),	/* 0 */
/* 94 */	NdrFcShort( 0x8 ),	/* 8 */
/* 96 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 98 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 100 */	NdrFcShort( 0x0 ),	/* 0 */
/* 102 */	NdrFcShort( 0x0 ),	/* 0 */
/* 104 */	NdrFcShort( 0x0 ),	/* 0 */
/* 106 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter instance */


	/* Parameter next */


	/* Parameter app_bundle */

/* 108 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 110 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 112 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Return value */


	/* Return value */


	/* Return value */

/* 114 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 116 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 118 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_version */


	/* Procedure get_appId */


	/* Procedure get_version */


	/* Procedure get_displayName */

/* 120 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 122 */	NdrFcLong( 0x0 ),	/* 0 */
/* 126 */	NdrFcShort( 0x7 ),	/* 7 */
/* 128 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 130 */	NdrFcShort( 0x0 ),	/* 0 */
/* 132 */	NdrFcShort( 0x8 ),	/* 8 */
/* 134 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 136 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 138 */	NdrFcShort( 0x1 ),	/* 1 */
/* 140 */	NdrFcShort( 0x0 ),	/* 0 */
/* 142 */	NdrFcShort( 0x0 ),	/* 0 */
/* 144 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IAppVersionWeb0000 */


	/* Parameter __MIDL__IAppWeb0000 */


	/* Parameter __MIDL__IAppVersion0000 */


	/* Parameter __MIDL__IAppBundle0000 */

/* 146 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 148 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 150 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Return value */


	/* Return value */


	/* Return value */


	/* Return value */

/* 152 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 154 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 156 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure createInstalledApp */


	/* Procedure put_displayName */

/* 158 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 160 */	NdrFcLong( 0x0 ),	/* 0 */
/* 164 */	NdrFcShort( 0x8 ),	/* 8 */
/* 166 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 168 */	NdrFcShort( 0x0 ),	/* 0 */
/* 170 */	NdrFcShort( 0x8 ),	/* 8 */
/* 172 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 174 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 176 */	NdrFcShort( 0x0 ),	/* 0 */
/* 178 */	NdrFcShort( 0x1 ),	/* 1 */
/* 180 */	NdrFcShort( 0x0 ),	/* 0 */
/* 182 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter app_id */


	/* Parameter __MIDL__IAppBundle0001 */

/* 184 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 186 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 188 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Return value */


	/* Return value */

/* 190 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 192 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 194 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_output */


	/* Procedure get_filename */


	/* Procedure get_appId */


	/* Procedure get_displayLanguage */

/* 196 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 198 */	NdrFcLong( 0x0 ),	/* 0 */
/* 202 */	NdrFcShort( 0x9 ),	/* 9 */
/* 204 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 206 */	NdrFcShort( 0x0 ),	/* 0 */
/* 208 */	NdrFcShort( 0x8 ),	/* 8 */
/* 210 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 212 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 214 */	NdrFcShort( 0x1 ),	/* 1 */
/* 216 */	NdrFcShort( 0x0 ),	/* 0 */
/* 218 */	NdrFcShort( 0x0 ),	/* 0 */
/* 220 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IAppCommandWeb0002 */


	/* Parameter __MIDL__IPackage0001 */


	/* Parameter __MIDL__IApp0000 */


	/* Parameter __MIDL__IAppBundle0002 */

/* 222 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 224 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 226 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Return value */


	/* Return value */


	/* Return value */


	/* Return value */

/* 228 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 230 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 232 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_displayLanguage */

/* 234 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 236 */	NdrFcLong( 0x0 ),	/* 0 */
/* 240 */	NdrFcShort( 0xa ),	/* 10 */
/* 242 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 244 */	NdrFcShort( 0x0 ),	/* 0 */
/* 246 */	NdrFcShort( 0x8 ),	/* 8 */
/* 248 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 250 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 252 */	NdrFcShort( 0x0 ),	/* 0 */
/* 254 */	NdrFcShort( 0x1 ),	/* 1 */
/* 256 */	NdrFcShort( 0x0 ),	/* 0 */
/* 258 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IAppBundle0003 */

/* 260 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 262 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 264 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Return value */

/* 266 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 268 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 270 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_output */


	/* Procedure get_installSource */

/* 272 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 274 */	NdrFcLong( 0x0 ),	/* 0 */
/* 278 */	NdrFcShort( 0xb ),	/* 11 */
/* 280 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 282 */	NdrFcShort( 0x0 ),	/* 0 */
/* 284 */	NdrFcShort( 0x8 ),	/* 8 */
/* 286 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 288 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 290 */	NdrFcShort( 0x1 ),	/* 1 */
/* 292 */	NdrFcShort( 0x0 ),	/* 0 */
/* 294 */	NdrFcShort( 0x0 ),	/* 0 */
/* 296 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IAppCommand20000 */


	/* Parameter __MIDL__IAppBundle0004 */

/* 298 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 300 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 302 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Return value */


	/* Return value */

/* 304 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 306 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 308 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_installSource */

/* 310 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 312 */	NdrFcLong( 0x0 ),	/* 0 */
/* 316 */	NdrFcShort( 0xc ),	/* 12 */
/* 318 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 320 */	NdrFcShort( 0x0 ),	/* 0 */
/* 322 */	NdrFcShort( 0x8 ),	/* 8 */
/* 324 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 326 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 328 */	NdrFcShort( 0x0 ),	/* 0 */
/* 330 */	NdrFcShort( 0x1 ),	/* 1 */
/* 332 */	NdrFcShort( 0x0 ),	/* 0 */
/* 334 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IAppBundle0005 */

/* 336 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 338 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 340 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Return value */

/* 342 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 344 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 346 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_originURL */

/* 348 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 350 */	NdrFcLong( 0x0 ),	/* 0 */
/* 354 */	NdrFcShort( 0xd ),	/* 13 */
/* 356 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 358 */	NdrFcShort( 0x0 ),	/* 0 */
/* 360 */	NdrFcShort( 0x8 ),	/* 8 */
/* 362 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 364 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 366 */	NdrFcShort( 0x1 ),	/* 1 */
/* 368 */	NdrFcShort( 0x0 ),	/* 0 */
/* 370 */	NdrFcShort( 0x0 ),	/* 0 */
/* 372 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IAppBundle0006 */

/* 374 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 376 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 378 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Return value */

/* 380 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 382 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 384 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_originURL */

/* 386 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 388 */	NdrFcLong( 0x0 ),	/* 0 */
/* 392 */	NdrFcShort( 0xe ),	/* 14 */
/* 394 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 396 */	NdrFcShort( 0x0 ),	/* 0 */
/* 398 */	NdrFcShort( 0x8 ),	/* 8 */
/* 400 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 402 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 404 */	NdrFcShort( 0x0 ),	/* 0 */
/* 406 */	NdrFcShort( 0x1 ),	/* 1 */
/* 408 */	NdrFcShort( 0x0 ),	/* 0 */
/* 410 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IAppBundle0007 */

/* 412 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 414 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 416 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Return value */

/* 418 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 420 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 422 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_serverInstallDataIndex */


	/* Procedure get_offlineDirectory */

/* 424 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 426 */	NdrFcLong( 0x0 ),	/* 0 */
/* 430 */	NdrFcShort( 0xf ),	/* 15 */
/* 432 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 434 */	NdrFcShort( 0x0 ),	/* 0 */
/* 436 */	NdrFcShort( 0x8 ),	/* 8 */
/* 438 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 440 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 442 */	NdrFcShort( 0x1 ),	/* 1 */
/* 444 */	NdrFcShort( 0x0 ),	/* 0 */
/* 446 */	NdrFcShort( 0x0 ),	/* 0 */
/* 448 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IAppWeb0001 */


	/* Parameter offline_dir */

/* 450 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 452 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 454 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Return value */


	/* Return value */

/* 456 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 458 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 460 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_serverInstallDataIndex */


	/* Procedure put_offlineDirectory */

/* 462 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 464 */	NdrFcLong( 0x0 ),	/* 0 */
/* 468 */	NdrFcShort( 0x10 ),	/* 16 */
/* 470 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 472 */	NdrFcShort( 0x0 ),	/* 0 */
/* 474 */	NdrFcShort( 0x8 ),	/* 8 */
/* 476 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 478 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 480 */	NdrFcShort( 0x0 ),	/* 0 */
/* 482 */	NdrFcShort( 0x1 ),	/* 1 */
/* 484 */	NdrFcShort( 0x0 ),	/* 0 */
/* 486 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IAppWeb0002 */


	/* Parameter offline_dir */

/* 488 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 490 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 492 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Return value */


	/* Return value */

/* 494 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 496 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 498 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_sessionId */

/* 500 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 502 */	NdrFcLong( 0x0 ),	/* 0 */
/* 506 */	NdrFcShort( 0x11 ),	/* 17 */
/* 508 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 510 */	NdrFcShort( 0x0 ),	/* 0 */
/* 512 */	NdrFcShort( 0x8 ),	/* 8 */
/* 514 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 516 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 518 */	NdrFcShort( 0x1 ),	/* 1 */
/* 520 */	NdrFcShort( 0x0 ),	/* 0 */
/* 522 */	NdrFcShort( 0x0 ),	/* 0 */
/* 524 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter session_id */

/* 526 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 528 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 530 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Return value */

/* 532 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 534 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 536 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_sessionId */

/* 538 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 540 */	NdrFcLong( 0x0 ),	/* 0 */
/* 544 */	NdrFcShort( 0x12 ),	/* 18 */
/* 546 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 548 */	NdrFcShort( 0x0 ),	/* 0 */
/* 550 */	NdrFcShort( 0x8 ),	/* 8 */
/* 552 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 554 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 556 */	NdrFcShort( 0x0 ),	/* 0 */
/* 558 */	NdrFcShort( 0x1 ),	/* 1 */
/* 560 */	NdrFcShort( 0x0 ),	/* 0 */
/* 562 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter session_id */

/* 564 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 566 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 568 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Return value */

/* 570 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 572 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 574 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_sendPings */

/* 576 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 578 */	NdrFcLong( 0x0 ),	/* 0 */
/* 582 */	NdrFcShort( 0x13 ),	/* 19 */
/* 584 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 586 */	NdrFcShort( 0x0 ),	/* 0 */
/* 588 */	NdrFcShort( 0x22 ),	/* 34 */
/* 590 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 592 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 594 */	NdrFcShort( 0x0 ),	/* 0 */
/* 596 */	NdrFcShort( 0x0 ),	/* 0 */
/* 598 */	NdrFcShort( 0x0 ),	/* 0 */
/* 600 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter send_pings */

/* 602 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 604 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 606 */	0x6,		/* FC_SHORT */
			0x0,		/* 0 */

	/* Return value */

/* 608 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 610 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 612 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_sendPings */

/* 614 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 616 */	NdrFcLong( 0x0 ),	/* 0 */
/* 620 */	NdrFcShort( 0x14 ),	/* 20 */
/* 622 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 624 */	NdrFcShort( 0x6 ),	/* 6 */
/* 626 */	NdrFcShort( 0x8 ),	/* 8 */
/* 628 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 630 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 632 */	NdrFcShort( 0x0 ),	/* 0 */
/* 634 */	NdrFcShort( 0x0 ),	/* 0 */
/* 636 */	NdrFcShort( 0x0 ),	/* 0 */
/* 638 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter send_pings */

/* 640 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 642 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 644 */	0x6,		/* FC_SHORT */
			0x0,		/* 0 */

	/* Return value */

/* 646 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 648 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 650 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_priority */

/* 652 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 654 */	NdrFcLong( 0x0 ),	/* 0 */
/* 658 */	NdrFcShort( 0x15 ),	/* 21 */
/* 660 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 662 */	NdrFcShort( 0x0 ),	/* 0 */
/* 664 */	NdrFcShort( 0x24 ),	/* 36 */
/* 666 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 668 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 670 */	NdrFcShort( 0x0 ),	/* 0 */
/* 672 */	NdrFcShort( 0x0 ),	/* 0 */
/* 674 */	NdrFcShort( 0x0 ),	/* 0 */
/* 676 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter priority */

/* 678 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 680 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 682 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 684 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 686 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 688 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_priority */

/* 690 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 692 */	NdrFcLong( 0x0 ),	/* 0 */
/* 696 */	NdrFcShort( 0x16 ),	/* 22 */
/* 698 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 700 */	NdrFcShort( 0x8 ),	/* 8 */
/* 702 */	NdrFcShort( 0x8 ),	/* 8 */
/* 704 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 706 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 708 */	NdrFcShort( 0x0 ),	/* 0 */
/* 710 */	NdrFcShort( 0x0 ),	/* 0 */
/* 712 */	NdrFcShort( 0x0 ),	/* 0 */
/* 714 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter priority */

/* 716 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 718 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 720 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 722 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 724 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 726 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_postInstallAction */


	/* Procedure get_Count */

/* 728 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 730 */	NdrFcLong( 0x0 ),	/* 0 */
/* 734 */	NdrFcShort( 0x17 ),	/* 23 */
/* 736 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 738 */	NdrFcShort( 0x0 ),	/* 0 */
/* 740 */	NdrFcShort( 0x24 ),	/* 36 */
/* 742 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 744 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 746 */	NdrFcShort( 0x0 ),	/* 0 */
/* 748 */	NdrFcShort( 0x0 ),	/* 0 */
/* 750 */	NdrFcShort( 0x0 ),	/* 0 */
/* 752 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__ICurrentState0015 */


	/* Parameter count */

/* 754 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 756 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 758 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */


	/* Return value */

/* 760 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 762 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 764 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_Item */

/* 766 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 768 */	NdrFcLong( 0x0 ),	/* 0 */
/* 772 */	NdrFcShort( 0x18 ),	/* 24 */
/* 774 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 776 */	NdrFcShort( 0x8 ),	/* 8 */
/* 778 */	NdrFcShort( 0x8 ),	/* 8 */
/* 780 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x3,		/* 3 */
/* 782 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 784 */	NdrFcShort( 0x0 ),	/* 0 */
/* 786 */	NdrFcShort( 0x0 ),	/* 0 */
/* 788 */	NdrFcShort( 0x0 ),	/* 0 */
/* 790 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter index */

/* 792 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 794 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 796 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter app */

/* 798 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 800 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 802 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Return value */

/* 804 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 806 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 808 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_altTokens */

/* 810 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 812 */	NdrFcLong( 0x0 ),	/* 0 */
/* 816 */	NdrFcShort( 0x19 ),	/* 25 */
/* 818 */	NdrFcShort( 0x28 ),	/* X64 Stack size/offset = 40 */
/* 820 */	NdrFcShort( 0x18 ),	/* 24 */
/* 822 */	NdrFcShort( 0x8 ),	/* 8 */
/* 824 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x4,		/* 4 */
/* 826 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 828 */	NdrFcShort( 0x0 ),	/* 0 */
/* 830 */	NdrFcShort( 0x0 ),	/* 0 */
/* 832 */	NdrFcShort( 0x0 ),	/* 0 */
/* 834 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter impersonation_token */

/* 836 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 838 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 840 */	0xb9,		/* FC_UINT3264 */
			0x0,		/* 0 */

	/* Parameter primary_token */

/* 842 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 844 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 846 */	0xb9,		/* FC_UINT3264 */
			0x0,		/* 0 */

	/* Parameter caller_proc_id */

/* 848 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 850 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 852 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 854 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 856 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 858 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_parentHWND */

/* 860 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 862 */	NdrFcLong( 0x0 ),	/* 0 */
/* 866 */	NdrFcShort( 0x1a ),	/* 26 */
/* 868 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 870 */	NdrFcShort( 0x8 ),	/* 8 */
/* 872 */	NdrFcShort( 0x8 ),	/* 8 */
/* 874 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 876 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 878 */	NdrFcShort( 0x0 ),	/* 0 */
/* 880 */	NdrFcShort( 0x0 ),	/* 0 */
/* 882 */	NdrFcShort( 0x0 ),	/* 0 */
/* 884 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter hwnd */

/* 886 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 888 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 890 */	0xb9,		/* FC_UINT3264 */
			0x0,		/* 0 */

	/* Return value */

/* 892 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 894 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 896 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure initialize */

/* 898 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 900 */	NdrFcLong( 0x0 ),	/* 0 */
/* 904 */	NdrFcShort( 0x1b ),	/* 27 */
/* 906 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 908 */	NdrFcShort( 0x0 ),	/* 0 */
/* 910 */	NdrFcShort( 0x8 ),	/* 8 */
/* 912 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 914 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 916 */	NdrFcShort( 0x0 ),	/* 0 */
/* 918 */	NdrFcShort( 0x0 ),	/* 0 */
/* 920 */	NdrFcShort( 0x0 ),	/* 0 */
/* 922 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 924 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 926 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 928 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_command */


	/* Procedure createApp */

/* 930 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 932 */	NdrFcLong( 0x0 ),	/* 0 */
/* 936 */	NdrFcShort( 0x1c ),	/* 28 */
/* 938 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 940 */	NdrFcShort( 0x0 ),	/* 0 */
/* 942 */	NdrFcShort( 0x8 ),	/* 8 */
/* 944 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 946 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 948 */	NdrFcShort( 0x0 ),	/* 0 */
/* 950 */	NdrFcShort( 0x1 ),	/* 1 */
/* 952 */	NdrFcShort( 0x0 ),	/* 0 */
/* 954 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter command_id */


	/* Parameter app_id */

/* 956 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 958 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 960 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Parameter command */


	/* Parameter app */

/* 962 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 964 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 966 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Return value */


	/* Return value */

/* 968 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 970 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 972 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure createInstalledApp */

/* 974 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 976 */	NdrFcLong( 0x0 ),	/* 0 */
/* 980 */	NdrFcShort( 0x1d ),	/* 29 */
/* 982 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 984 */	NdrFcShort( 0x0 ),	/* 0 */
/* 986 */	NdrFcShort( 0x8 ),	/* 8 */
/* 988 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 990 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 992 */	NdrFcShort( 0x0 ),	/* 0 */
/* 994 */	NdrFcShort( 0x1 ),	/* 1 */
/* 996 */	NdrFcShort( 0x0 ),	/* 0 */
/* 998 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter app_id */

/* 1000 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 1002 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 1004 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Parameter app */

/* 1006 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 1008 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 1010 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Return value */

/* 1012 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1014 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 1016 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure createAllInstalledApps */

/* 1018 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1020 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1024 */	NdrFcShort( 0x1e ),	/* 30 */
/* 1026 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 1028 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1030 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1032 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 1034 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1036 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1038 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1040 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1042 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 1044 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1046 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 1048 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure checkForUpdate */

/* 1050 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1052 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1056 */	NdrFcShort( 0x1f ),	/* 31 */
/* 1058 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 1060 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1062 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1064 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 1066 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1068 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1070 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1072 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1074 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 1076 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1078 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 1080 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure download */

/* 1082 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1084 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1088 */	NdrFcShort( 0x20 ),	/* 32 */
/* 1090 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 1092 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1094 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1096 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 1098 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1100 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1102 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1104 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1106 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 1108 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1110 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 1112 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure install */

/* 1114 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1116 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1120 */	NdrFcShort( 0x21 ),	/* 33 */
/* 1122 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 1124 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1126 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1128 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 1130 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1132 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1134 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1136 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1138 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 1140 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1142 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 1144 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure updateAllApps */

/* 1146 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1148 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1152 */	NdrFcShort( 0x22 ),	/* 34 */
/* 1154 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 1156 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1158 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1160 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 1162 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1164 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1166 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1168 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1170 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 1172 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1174 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 1176 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure stop */

/* 1178 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1180 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1184 */	NdrFcShort( 0x23 ),	/* 35 */
/* 1186 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 1188 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1190 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1192 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 1194 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1196 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1198 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1200 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1202 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 1204 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1206 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 1208 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure pause */

/* 1210 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1212 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1216 */	NdrFcShort( 0x24 ),	/* 36 */
/* 1218 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 1220 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1222 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1224 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 1226 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1228 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1230 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1232 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1234 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 1236 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1238 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 1240 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure resume */

/* 1242 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1244 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1248 */	NdrFcShort( 0x25 ),	/* 37 */
/* 1250 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 1252 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1254 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1256 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 1258 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1260 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1262 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1264 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1266 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 1268 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1270 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 1272 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure isBusy */

/* 1274 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1276 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1280 */	NdrFcShort( 0x26 ),	/* 38 */
/* 1282 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 1284 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1286 */	NdrFcShort( 0x22 ),	/* 34 */
/* 1288 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 1290 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1292 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1294 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1296 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1298 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter is_busy */

/* 1300 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 1302 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 1304 */	0x6,		/* FC_SHORT */
			0x0,		/* 0 */

	/* Return value */

/* 1306 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1308 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 1310 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure downloadPackage */

/* 1312 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1314 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1318 */	NdrFcShort( 0x27 ),	/* 39 */
/* 1320 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 1322 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1324 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1326 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 1328 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 1330 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1332 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1334 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1336 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter app_id */

/* 1338 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 1340 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 1342 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Parameter package_name */

/* 1344 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 1346 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 1348 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Return value */

/* 1350 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1352 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 1354 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_currentState */

/* 1356 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1358 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1362 */	NdrFcShort( 0x28 ),	/* 40 */
/* 1364 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 1366 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1368 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1370 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 1372 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 1374 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1376 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1378 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1380 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter current_state */

/* 1382 */	NdrFcShort( 0x6113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=24 */
/* 1384 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 1386 */	NdrFcShort( 0x3fa ),	/* Type Offset=1018 */

	/* Return value */

/* 1388 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1390 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 1392 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure createAppBundleWeb */


	/* Procedure get_currentVersion */

/* 1394 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1396 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1400 */	NdrFcShort( 0x7 ),	/* 7 */
/* 1402 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 1404 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1406 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1408 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 1410 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1412 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1414 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1416 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1418 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter app_bundle_web */


	/* Parameter current */

/* 1420 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 1422 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 1424 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Return value */


	/* Return value */

/* 1426 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1428 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 1430 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_currentVersionWeb */


	/* Procedure get_nextVersion */

/* 1432 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1434 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1438 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1440 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 1442 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1444 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1446 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 1448 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1450 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1452 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1454 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1456 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter current */


	/* Parameter next */

/* 1458 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 1460 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 1462 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Return value */


	/* Return value */

/* 1464 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1466 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 1468 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_displayLanguage */


	/* Procedure get_displayName */

/* 1470 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1472 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1476 */	NdrFcShort( 0xa ),	/* 10 */
/* 1478 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 1480 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1482 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1484 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 1486 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 1488 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1490 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1492 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1494 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IAppBundleWeb0000 */


	/* Parameter __MIDL__IApp0001 */

/* 1496 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 1498 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 1500 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Return value */


	/* Return value */

/* 1502 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1504 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 1506 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_displayLanguage */


	/* Procedure put_displayName */

/* 1508 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1510 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1514 */	NdrFcShort( 0xb ),	/* 11 */
/* 1516 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 1518 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1520 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1522 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 1524 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 1526 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1528 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1530 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1532 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IAppBundleWeb0001 */


	/* Parameter __MIDL__IApp0002 */

/* 1534 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 1536 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 1538 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Return value */


	/* Return value */

/* 1540 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1542 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 1544 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_language */

/* 1546 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1548 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1552 */	NdrFcShort( 0xc ),	/* 12 */
/* 1554 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 1556 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1558 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1560 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 1562 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 1564 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1566 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1568 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1570 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0003 */

/* 1572 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 1574 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 1576 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Return value */

/* 1578 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1580 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 1582 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_language */

/* 1584 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1586 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1590 */	NdrFcShort( 0xd ),	/* 13 */
/* 1592 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 1594 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1596 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1598 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 1600 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 1602 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1604 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1606 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1608 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0004 */

/* 1610 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 1612 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 1614 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Return value */

/* 1616 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1618 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 1620 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_ap */

/* 1622 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1624 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1628 */	NdrFcShort( 0xe ),	/* 14 */
/* 1630 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 1632 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1634 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1636 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 1638 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 1640 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1642 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1644 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1646 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0005 */

/* 1648 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 1650 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 1652 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Return value */

/* 1654 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1656 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 1658 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_ap */

/* 1660 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1662 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1666 */	NdrFcShort( 0xf ),	/* 15 */
/* 1668 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 1670 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1672 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1674 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 1676 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 1678 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1680 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1682 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1684 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0006 */

/* 1686 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 1688 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 1690 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Return value */

/* 1692 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1694 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 1696 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_ttToken */

/* 1698 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1700 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1704 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1706 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 1708 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1710 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1712 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 1714 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 1716 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1718 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1720 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1722 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0007 */

/* 1724 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 1726 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 1728 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Return value */

/* 1730 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1732 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 1734 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_ttToken */

/* 1736 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1738 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1742 */	NdrFcShort( 0x11 ),	/* 17 */
/* 1744 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 1746 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1748 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1750 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 1752 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 1754 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1756 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1758 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1760 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0008 */

/* 1762 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 1764 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 1766 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Return value */

/* 1768 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1770 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 1772 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_completionMessage */


	/* Procedure get_iid */

/* 1774 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1776 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1780 */	NdrFcShort( 0x12 ),	/* 18 */
/* 1782 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 1784 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1786 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1788 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 1790 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 1792 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1794 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1796 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1798 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__ICurrentState0010 */


	/* Parameter __MIDL__IApp0009 */

/* 1800 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 1802 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 1804 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Return value */


	/* Return value */

/* 1806 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1808 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 1810 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_iid */

/* 1812 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1814 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1818 */	NdrFcShort( 0x13 ),	/* 19 */
/* 1820 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 1822 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1824 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1826 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 1828 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 1830 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1832 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1834 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1836 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0010 */

/* 1838 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 1840 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 1842 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Return value */

/* 1844 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1846 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 1848 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_brandCode */

/* 1850 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1852 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1856 */	NdrFcShort( 0x14 ),	/* 20 */
/* 1858 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 1860 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1862 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1864 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 1866 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 1868 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1870 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1872 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1874 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0011 */

/* 1876 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 1878 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 1880 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Return value */

/* 1882 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1884 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 1886 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_brandCode */

/* 1888 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1890 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1894 */	NdrFcShort( 0x15 ),	/* 21 */
/* 1896 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 1898 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1900 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1902 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 1904 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 1906 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1908 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1910 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1912 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0012 */

/* 1914 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 1916 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 1918 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Return value */

/* 1920 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1922 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 1924 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_postInstallUrl */


	/* Procedure get_clientId */

/* 1926 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1928 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1932 */	NdrFcShort( 0x16 ),	/* 22 */
/* 1934 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 1936 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1938 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1940 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 1942 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 1944 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1946 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1948 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1950 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__ICurrentState0014 */


	/* Parameter __MIDL__IApp0013 */

/* 1952 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 1954 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 1956 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Return value */


	/* Return value */

/* 1958 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1960 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 1962 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_clientId */

/* 1964 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1966 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1970 */	NdrFcShort( 0x17 ),	/* 23 */
/* 1972 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 1974 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1976 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1978 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 1980 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 1982 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1984 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1986 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1988 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0014 */

/* 1990 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 1992 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 1994 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Return value */

/* 1996 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1998 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 2000 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_labels */

/* 2002 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2004 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2008 */	NdrFcShort( 0x18 ),	/* 24 */
/* 2010 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 2012 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2014 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2016 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 2018 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 2020 */	NdrFcShort( 0x1 ),	/* 1 */
/* 2022 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2024 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2026 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0015 */

/* 2028 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 2030 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 2032 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Return value */

/* 2034 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2036 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 2038 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_labels */

/* 2040 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2042 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2046 */	NdrFcShort( 0x19 ),	/* 25 */
/* 2048 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 2050 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2052 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2054 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 2056 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 2058 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2060 */	NdrFcShort( 0x1 ),	/* 1 */
/* 2062 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2064 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0016 */

/* 2066 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 2068 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 2070 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Return value */

/* 2072 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2074 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 2076 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_referralId */

/* 2078 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2080 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2084 */	NdrFcShort( 0x1a ),	/* 26 */
/* 2086 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 2088 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2090 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2092 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 2094 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 2096 */	NdrFcShort( 0x1 ),	/* 1 */
/* 2098 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2100 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2102 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0017 */

/* 2104 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 2106 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 2108 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Return value */

/* 2110 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2112 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 2114 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_referralId */

/* 2116 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2118 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2122 */	NdrFcShort( 0x1b ),	/* 27 */
/* 2124 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 2126 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2128 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2130 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 2132 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 2134 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2136 */	NdrFcShort( 0x1 ),	/* 1 */
/* 2138 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2140 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0018 */

/* 2142 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 2144 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 2146 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Return value */

/* 2148 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2150 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 2152 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_browserType */

/* 2154 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2156 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2160 */	NdrFcShort( 0x1d ),	/* 29 */
/* 2162 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 2164 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2166 */	NdrFcShort( 0x24 ),	/* 36 */
/* 2168 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 2170 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2172 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2174 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2176 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2178 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0019 */

/* 2180 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2182 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 2184 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2186 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2188 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 2190 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_browserType */

/* 2192 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2194 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2198 */	NdrFcShort( 0x1e ),	/* 30 */
/* 2200 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 2202 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2204 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2206 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 2208 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2210 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2212 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2214 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2216 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0020 */

/* 2218 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2220 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 2222 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2224 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2226 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 2228 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_clientInstallData */

/* 2230 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2232 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2236 */	NdrFcShort( 0x1f ),	/* 31 */
/* 2238 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 2240 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2242 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2244 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 2246 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 2248 */	NdrFcShort( 0x1 ),	/* 1 */
/* 2250 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2252 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2254 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0021 */

/* 2256 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 2258 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 2260 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Return value */

/* 2262 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2264 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 2266 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_clientInstallData */

/* 2268 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2270 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2274 */	NdrFcShort( 0x20 ),	/* 32 */
/* 2276 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 2278 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2280 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2282 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 2284 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 2286 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2288 */	NdrFcShort( 0x1 ),	/* 1 */
/* 2290 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2292 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0022 */

/* 2294 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 2296 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 2298 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Return value */

/* 2300 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2302 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 2304 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_serverInstallDataIndex */

/* 2306 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2308 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2312 */	NdrFcShort( 0x21 ),	/* 33 */
/* 2314 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 2316 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2318 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2320 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 2322 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 2324 */	NdrFcShort( 0x1 ),	/* 1 */
/* 2326 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2328 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2330 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0023 */

/* 2332 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 2334 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 2336 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Return value */

/* 2338 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2340 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 2342 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_serverInstallDataIndex */

/* 2344 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2346 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2350 */	NdrFcShort( 0x22 ),	/* 34 */
/* 2352 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 2354 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2356 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2358 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 2360 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 2362 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2364 */	NdrFcShort( 0x1 ),	/* 1 */
/* 2366 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2368 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0024 */

/* 2370 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 2372 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 2374 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Return value */

/* 2376 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2378 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 2380 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_isEulaAccepted */

/* 2382 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2384 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2388 */	NdrFcShort( 0x23 ),	/* 35 */
/* 2390 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 2392 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2394 */	NdrFcShort( 0x22 ),	/* 34 */
/* 2396 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 2398 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2400 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2402 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2404 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2406 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0025 */

/* 2408 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2410 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 2412 */	0x6,		/* FC_SHORT */
			0x0,		/* 0 */

	/* Return value */

/* 2414 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2416 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 2418 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_isEulaAccepted */

/* 2420 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2422 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2426 */	NdrFcShort( 0x24 ),	/* 36 */
/* 2428 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 2430 */	NdrFcShort( 0x6 ),	/* 6 */
/* 2432 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2434 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 2436 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2438 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2440 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2442 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2444 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0026 */

/* 2446 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2448 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 2450 */	0x6,		/* FC_SHORT */
			0x0,		/* 0 */

	/* Return value */

/* 2452 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2454 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 2456 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_usageStatsEnable */

/* 2458 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2460 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2464 */	NdrFcShort( 0x25 ),	/* 37 */
/* 2466 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 2468 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2470 */	NdrFcShort( 0x24 ),	/* 36 */
/* 2472 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 2474 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2476 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2478 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2480 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2482 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0027 */

/* 2484 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2486 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 2488 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2490 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2492 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 2494 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_usageStatsEnable */

/* 2496 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2498 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2502 */	NdrFcShort( 0x26 ),	/* 38 */
/* 2504 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 2506 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2508 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2510 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 2512 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2514 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2516 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2518 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2520 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0028 */

/* 2522 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2524 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 2526 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2528 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2530 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 2532 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_installTimeDiffSec */

/* 2534 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2536 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2540 */	NdrFcShort( 0x27 ),	/* 39 */
/* 2542 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 2544 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2546 */	NdrFcShort( 0x24 ),	/* 36 */
/* 2548 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 2550 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2552 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2554 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2556 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2558 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0029 */

/* 2560 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2562 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 2564 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2566 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2568 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 2570 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_currentState */

/* 2572 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2574 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2578 */	NdrFcShort( 0x28 ),	/* 40 */
/* 2580 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 2582 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2584 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2586 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 2588 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2590 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2592 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2594 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2596 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0030 */

/* 2598 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 2600 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 2602 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Return value */

/* 2604 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2606 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 2608 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_untrustedData */

/* 2610 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2612 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2616 */	NdrFcShort( 0x29 ),	/* 41 */
/* 2618 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 2620 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2622 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2624 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 2626 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 2628 */	NdrFcShort( 0x1 ),	/* 1 */
/* 2630 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2632 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2634 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp20000 */

/* 2636 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 2638 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 2640 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Return value */

/* 2642 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2644 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 2646 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_untrustedData */

/* 2648 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2650 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2654 */	NdrFcShort( 0x2a ),	/* 42 */
/* 2656 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 2658 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2660 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2662 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 2664 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 2666 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2668 */	NdrFcShort( 0x1 ),	/* 1 */
/* 2670 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2672 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp20001 */

/* 2674 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 2676 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 2678 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Return value */

/* 2680 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2682 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 2684 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_isDone */


	/* Procedure get_isWebAccessible */

/* 2686 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2688 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2692 */	NdrFcShort( 0x7 ),	/* 7 */
/* 2694 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 2696 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2698 */	NdrFcShort( 0x22 ),	/* 34 */
/* 2700 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 2702 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2704 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2706 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2708 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2710 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter is_done */


	/* Parameter __MIDL__IAppCommand0000 */

/* 2712 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2714 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 2716 */	0x6,		/* FC_SHORT */
			0x0,		/* 0 */

	/* Return value */


	/* Return value */

/* 2718 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2720 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 2722 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_completionHResult */


	/* Procedure get_packageCount */


	/* Procedure get_exitCode */


	/* Procedure get_packageCount */


	/* Procedure get_status */

/* 2724 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2726 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2730 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2732 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 2734 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2736 */	NdrFcShort( 0x24 ),	/* 36 */
/* 2738 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 2740 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2742 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2744 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2746 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2748 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter hr */


	/* Parameter count */


	/* Parameter __MIDL__IAppCommandWeb0001 */


	/* Parameter count */


	/* Parameter __MIDL__IAppCommand0001 */

/* 2750 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2752 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 2754 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */


	/* Return value */


	/* Return value */


	/* Return value */


	/* Return value */

/* 2756 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2758 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 2760 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_bytesDownloaded */


	/* Procedure get_exitCode */

/* 2762 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2764 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2768 */	NdrFcShort( 0x9 ),	/* 9 */
/* 2770 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 2772 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2774 */	NdrFcShort( 0x24 ),	/* 36 */
/* 2776 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 2778 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2780 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2782 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2784 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2786 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__ICurrentState0002 */


	/* Parameter __MIDL__IAppCommand0002 */

/* 2788 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2790 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 2792 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */


	/* Return value */

/* 2794 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2796 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 2798 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure execute */


	/* Procedure execute */

/* 2800 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2802 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2806 */	NdrFcShort( 0xa ),	/* 10 */
/* 2808 */	NdrFcShort( 0x58 ),	/* X64 Stack size/offset = 88 */
/* 2810 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2812 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2814 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0xa,		/* 10 */
/* 2816 */	0xa,		/* 10 */
			0x85,		/* Ext Flags:  new corr desc, srv corr check, has big byval param */
/* 2818 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2820 */	NdrFcShort( 0x1 ),	/* 1 */
/* 2822 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2824 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter arg1 */


	/* Parameter arg1 */

/* 2826 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 2828 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 2830 */	NdrFcShort( 0x40c ),	/* Type Offset=1036 */

	/* Parameter arg2 */


	/* Parameter arg2 */

/* 2832 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 2834 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 2836 */	NdrFcShort( 0x40c ),	/* Type Offset=1036 */

	/* Parameter arg3 */


	/* Parameter arg3 */

/* 2838 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 2840 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 2842 */	NdrFcShort( 0x40c ),	/* Type Offset=1036 */

	/* Parameter arg4 */


	/* Parameter arg4 */

/* 2844 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 2846 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 2848 */	NdrFcShort( 0x40c ),	/* Type Offset=1036 */

	/* Parameter arg5 */


	/* Parameter arg5 */

/* 2850 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 2852 */	NdrFcShort( 0x28 ),	/* X64 Stack size/offset = 40 */
/* 2854 */	NdrFcShort( 0x40c ),	/* Type Offset=1036 */

	/* Parameter arg6 */


	/* Parameter arg6 */

/* 2856 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 2858 */	NdrFcShort( 0x30 ),	/* X64 Stack size/offset = 48 */
/* 2860 */	NdrFcShort( 0x40c ),	/* Type Offset=1036 */

	/* Parameter arg7 */


	/* Parameter arg7 */

/* 2862 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 2864 */	NdrFcShort( 0x38 ),	/* X64 Stack size/offset = 56 */
/* 2866 */	NdrFcShort( 0x40c ),	/* Type Offset=1036 */

	/* Parameter arg8 */


	/* Parameter arg8 */

/* 2868 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 2870 */	NdrFcShort( 0x40 ),	/* X64 Stack size/offset = 64 */
/* 2872 */	NdrFcShort( 0x40c ),	/* Type Offset=1036 */

	/* Parameter arg9 */


	/* Parameter arg9 */

/* 2874 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 2876 */	NdrFcShort( 0x48 ),	/* X64 Stack size/offset = 72 */
/* 2878 */	NdrFcShort( 0x40c ),	/* Type Offset=1036 */

	/* Return value */


	/* Return value */

/* 2880 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2882 */	NdrFcShort( 0x50 ),	/* X64 Stack size/offset = 80 */
/* 2884 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_packageWeb */


	/* Procedure get_package */

/* 2886 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2888 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2892 */	NdrFcShort( 0x9 ),	/* 9 */
/* 2894 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 2896 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2898 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2900 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x3,		/* 3 */
/* 2902 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2904 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2906 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2908 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2910 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter index */


	/* Parameter index */

/* 2912 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2914 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 2916 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter package */


	/* Parameter package */

/* 2918 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 2920 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 2922 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Return value */


	/* Return value */

/* 2924 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2926 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 2928 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get */

/* 2930 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2932 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2936 */	NdrFcShort( 0x7 ),	/* 7 */
/* 2938 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 2940 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2942 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2944 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 2946 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 2948 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2950 */	NdrFcShort( 0x1 ),	/* 1 */
/* 2952 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2954 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter dir */

/* 2956 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 2958 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 2960 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Return value */

/* 2962 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2964 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 2966 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_isAvailable */

/* 2968 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2970 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2974 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2976 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 2978 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2980 */	NdrFcShort( 0x22 ),	/* 34 */
/* 2982 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 2984 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2986 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2988 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2990 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2992 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IPackage0000 */

/* 2994 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2996 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 2998 */	0x6,		/* FC_SHORT */
			0x0,		/* 0 */

	/* Return value */

/* 3000 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3002 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 3004 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_availableVersion */

/* 3006 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3008 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3012 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3014 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 3016 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3018 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3020 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 3022 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 3024 */	NdrFcShort( 0x1 ),	/* 1 */
/* 3026 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3028 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3030 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__ICurrentState0001 */

/* 3032 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 3034 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 3036 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Return value */

/* 3038 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3040 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 3042 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_totalBytesToDownload */

/* 3044 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3046 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3050 */	NdrFcShort( 0xa ),	/* 10 */
/* 3052 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 3054 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3056 */	NdrFcShort( 0x24 ),	/* 36 */
/* 3058 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 3060 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 3062 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3064 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3066 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3068 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__ICurrentState0003 */

/* 3070 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 3072 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 3074 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 3076 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3078 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 3080 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_downloadTimeRemainingMs */

/* 3082 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3084 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3088 */	NdrFcShort( 0xb ),	/* 11 */
/* 3090 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 3092 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3094 */	NdrFcShort( 0x24 ),	/* 36 */
/* 3096 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 3098 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 3100 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3102 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3104 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3106 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__ICurrentState0004 */

/* 3108 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 3110 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 3112 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 3114 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3116 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 3118 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_nextRetryTime */

/* 3120 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3122 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3126 */	NdrFcShort( 0xc ),	/* 12 */
/* 3128 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 3130 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3132 */	NdrFcShort( 0x2c ),	/* 44 */
/* 3134 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 3136 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 3138 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3140 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3142 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3144 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__ICurrentState0005 */

/* 3146 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 3148 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 3150 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Return value */

/* 3152 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3154 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 3156 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_length */


	/* Procedure get_installProgress */

/* 3158 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3160 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3164 */	NdrFcShort( 0xd ),	/* 13 */
/* 3166 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 3168 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3170 */	NdrFcShort( 0x24 ),	/* 36 */
/* 3172 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 3174 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 3176 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3178 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3180 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3182 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter index */


	/* Parameter __MIDL__ICurrentState0006 */

/* 3184 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 3186 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 3188 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */


	/* Return value */

/* 3190 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3192 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 3194 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_installTimeRemainingMs */

/* 3196 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3198 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3202 */	NdrFcShort( 0xe ),	/* 14 */
/* 3204 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 3206 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3208 */	NdrFcShort( 0x24 ),	/* 36 */
/* 3210 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 3212 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 3214 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3216 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3218 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3220 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__ICurrentState0007 */

/* 3222 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 3224 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 3226 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 3228 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3230 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 3232 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_isCanceled */

/* 3234 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3236 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3240 */	NdrFcShort( 0xf ),	/* 15 */
/* 3242 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 3244 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3246 */	NdrFcShort( 0x22 ),	/* 34 */
/* 3248 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 3250 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 3252 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3254 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3256 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3258 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter is_canceled */

/* 3260 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 3262 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 3264 */	0x6,		/* FC_SHORT */
			0x0,		/* 0 */

	/* Return value */

/* 3266 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3268 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 3270 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_errorCode */

/* 3272 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3274 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3278 */	NdrFcShort( 0x10 ),	/* 16 */
/* 3280 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 3282 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3284 */	NdrFcShort( 0x24 ),	/* 36 */
/* 3286 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 3288 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 3290 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3292 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3294 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3296 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__ICurrentState0008 */

/* 3298 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 3300 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 3302 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 3304 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3306 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 3308 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_extraCode1 */

/* 3310 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3312 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3316 */	NdrFcShort( 0x11 ),	/* 17 */
/* 3318 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 3320 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3322 */	NdrFcShort( 0x24 ),	/* 36 */
/* 3324 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 3326 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 3328 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3330 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3332 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3334 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__ICurrentState0009 */

/* 3336 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 3338 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 3340 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 3342 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3344 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 3346 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_installerResultCode */

/* 3348 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3350 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3354 */	NdrFcShort( 0x13 ),	/* 19 */
/* 3356 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 3358 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3360 */	NdrFcShort( 0x24 ),	/* 36 */
/* 3362 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 3364 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 3366 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3368 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3370 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3372 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__ICurrentState0011 */

/* 3374 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 3376 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 3378 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 3380 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3382 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 3384 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_installerResultExtraCode1 */

/* 3386 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3388 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3392 */	NdrFcShort( 0x14 ),	/* 20 */
/* 3394 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 3396 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3398 */	NdrFcShort( 0x24 ),	/* 36 */
/* 3400 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 3402 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 3404 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3406 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3408 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3410 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__ICurrentState0012 */

/* 3412 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 3414 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 3416 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 3418 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3420 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 3422 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_postInstallLaunchCommandLine */

/* 3424 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3426 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3430 */	NdrFcShort( 0x15 ),	/* 21 */
/* 3432 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 3434 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3436 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3438 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 3440 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 3442 */	NdrFcShort( 0x1 ),	/* 1 */
/* 3444 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3446 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3448 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__ICurrentState0013 */

/* 3450 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 3452 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 3454 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Return value */

/* 3456 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3458 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 3460 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure UpdateRegistry */

/* 3462 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3464 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3468 */	NdrFcShort( 0x7 ),	/* 7 */
/* 3470 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 3472 */	NdrFcShort( 0x6 ),	/* 6 */
/* 3474 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3476 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 3478 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 3480 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3482 */	NdrFcShort( 0x1 ),	/* 1 */
/* 3484 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3486 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter app_id */

/* 3488 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 3490 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 3492 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Parameter is_machine */

/* 3494 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 3496 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 3498 */	0x6,		/* FC_SHORT */
			0x0,		/* 0 */

	/* Return value */

/* 3500 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3502 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 3504 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure QueryUserForCredentials */

/* 3506 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3508 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3512 */	NdrFcShort( 0x3 ),	/* 3 */
/* 3514 */	NdrFcShort( 0x38 ),	/* X64 Stack size/offset = 56 */
/* 3516 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3518 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3520 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x6,		/* 6 */
/* 3522 */	0xa,		/* 10 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 3524 */	NdrFcShort( 0x1 ),	/* 1 */
/* 3526 */	NdrFcShort( 0x1 ),	/* 1 */
/* 3528 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3530 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter owner_hwnd */

/* 3532 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 3534 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 3536 */	0xb9,		/* FC_UINT3264 */
			0x0,		/* 0 */

	/* Parameter server */

/* 3538 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 3540 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 3542 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Parameter message */

/* 3544 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 3546 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 3548 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Parameter username */

/* 3550 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 3552 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 3554 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Parameter password */

/* 3556 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 3558 */	NdrFcShort( 0x28 ),	/* X64 Stack size/offset = 40 */
/* 3560 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Return value */

/* 3562 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3564 */	NdrFcShort( 0x30 ),	/* X64 Stack size/offset = 48 */
/* 3566 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure setOriginURL */

/* 3568 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3570 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3574 */	NdrFcShort( 0x3 ),	/* 3 */
/* 3576 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 3578 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3580 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3582 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 3584 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 3586 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3588 */	NdrFcShort( 0x1 ),	/* 1 */
/* 3590 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3592 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter origin_url */

/* 3594 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 3596 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 3598 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Return value */

/* 3600 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3602 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 3604 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure createApp */

/* 3606 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3608 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3612 */	NdrFcShort( 0x7 ),	/* 7 */
/* 3614 */	NdrFcShort( 0x30 ),	/* X64 Stack size/offset = 48 */
/* 3616 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3618 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3620 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x5,		/* 5 */
/* 3622 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 3624 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3626 */	NdrFcShort( 0x1 ),	/* 1 */
/* 3628 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3630 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter app_guid */

/* 3632 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 3634 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 3636 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Parameter brand_code */

/* 3638 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 3640 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 3642 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Parameter language */

/* 3644 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 3646 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 3648 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Parameter ap */

/* 3650 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 3652 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 3654 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Return value */

/* 3656 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3658 */	NdrFcShort( 0x28 ),	/* X64 Stack size/offset = 40 */
/* 3660 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure OnInstalling */


	/* Procedure createAllInstalledApps */

/* 3662 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3664 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3668 */	NdrFcShort( 0x9 ),	/* 9 */
/* 3670 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 3672 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3674 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3676 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 3678 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 3680 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3682 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3684 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3686 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */


	/* Return value */

/* 3688 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3690 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 3692 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_parentHWND */

/* 3694 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3696 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3700 */	NdrFcShort( 0xc ),	/* 12 */
/* 3702 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 3704 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3706 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3708 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 3710 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 3712 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3714 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3716 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3718 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter hwnd */

/* 3720 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 3722 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 3724 */	0xb9,		/* FC_UINT3264 */
			0x0,		/* 0 */

	/* Return value */

/* 3726 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3728 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 3730 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_appWeb */

/* 3732 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3734 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3738 */	NdrFcShort( 0xe ),	/* 14 */
/* 3740 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 3742 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3744 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3746 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x3,		/* 3 */
/* 3748 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 3750 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3752 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3754 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3756 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter index */

/* 3758 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 3760 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 3762 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter app_web */

/* 3764 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 3766 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 3768 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Return value */

/* 3770 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3772 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 3774 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure initialize */

/* 3776 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3778 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3782 */	NdrFcShort( 0xf ),	/* 15 */
/* 3784 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 3786 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3788 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3790 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 3792 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 3794 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3796 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3798 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3800 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 3802 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3804 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 3806 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure checkForUpdate */

/* 3808 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3810 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3814 */	NdrFcShort( 0x10 ),	/* 16 */
/* 3816 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 3818 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3820 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3822 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 3824 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 3826 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3828 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3830 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3832 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 3834 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3836 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 3838 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure download */

/* 3840 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3842 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3846 */	NdrFcShort( 0x11 ),	/* 17 */
/* 3848 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 3850 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3852 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3854 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 3856 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 3858 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3860 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3862 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3864 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 3866 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3868 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 3870 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure install */

/* 3872 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3874 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3878 */	NdrFcShort( 0x12 ),	/* 18 */
/* 3880 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 3882 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3884 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3886 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 3888 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 3890 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3892 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3894 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3896 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 3898 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3900 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 3902 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure pause */

/* 3904 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3906 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3910 */	NdrFcShort( 0x13 ),	/* 19 */
/* 3912 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 3914 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3916 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3918 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 3920 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 3922 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3924 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3926 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3928 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 3930 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3932 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 3934 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure resume */

/* 3936 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3938 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3942 */	NdrFcShort( 0x14 ),	/* 20 */
/* 3944 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 3946 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3948 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3950 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 3952 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 3954 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3956 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3958 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3960 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 3962 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3964 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 3966 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure cancel */

/* 3968 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3970 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3974 */	NdrFcShort( 0x15 ),	/* 21 */
/* 3976 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 3978 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3980 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3982 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 3984 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 3986 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3988 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3990 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3992 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 3994 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3996 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 3998 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure downloadPackage */

/* 4000 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4002 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4006 */	NdrFcShort( 0x16 ),	/* 22 */
/* 4008 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 4010 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4012 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4014 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 4016 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 4018 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4020 */	NdrFcShort( 0x1 ),	/* 1 */
/* 4022 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4024 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter app_id */

/* 4026 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 4028 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 4030 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Parameter package_name */

/* 4032 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 4034 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 4036 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Return value */

/* 4038 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4040 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 4042 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_currentState */

/* 4044 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4046 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4050 */	NdrFcShort( 0x17 ),	/* 23 */
/* 4052 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 4054 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4056 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4058 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 4060 */	0xa,		/* 10 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 4062 */	NdrFcShort( 0x1 ),	/* 1 */
/* 4064 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4066 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4068 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter current_state */

/* 4070 */	NdrFcShort( 0x6113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=24 */
/* 4072 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 4074 */	NdrFcShort( 0x3fa ),	/* Type Offset=1018 */

	/* Return value */

/* 4076 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4078 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 4080 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_command */

/* 4082 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4084 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4088 */	NdrFcShort( 0xa ),	/* 10 */
/* 4090 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 4092 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4094 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4096 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 4098 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 4100 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4102 */	NdrFcShort( 0x1 ),	/* 1 */
/* 4104 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4106 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter command_id */

/* 4108 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 4110 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 4112 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Parameter command */

/* 4114 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 4116 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 4118 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Return value */

/* 4120 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4122 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 4124 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure cancel */

/* 4126 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4128 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4132 */	NdrFcShort( 0xb ),	/* 11 */
/* 4134 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 4136 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4138 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4140 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 4142 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 4144 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4146 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4148 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4150 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 4152 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4154 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 4156 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_currentState */

/* 4158 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4160 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4164 */	NdrFcShort( 0xc ),	/* 12 */
/* 4166 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 4168 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4170 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4172 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 4174 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 4176 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4178 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4180 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4182 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter current_state */

/* 4184 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 4186 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 4188 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Return value */

/* 4190 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4192 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 4194 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure launch */

/* 4196 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4198 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4202 */	NdrFcShort( 0xd ),	/* 13 */
/* 4204 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 4206 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4208 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4210 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 4212 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 4214 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4216 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4218 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4220 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 4222 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4224 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 4226 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure uninstall */

/* 4228 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4230 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4234 */	NdrFcShort( 0xe ),	/* 14 */
/* 4236 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 4238 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4240 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4242 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 4244 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 4246 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4248 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4250 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4252 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 4254 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4256 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 4258 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure createOmahaMachineServerAsync */

/* 4260 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4262 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4266 */	NdrFcShort( 0x3 ),	/* 3 */
/* 4268 */	NdrFcShort( 0x28 ),	/* X64 Stack size/offset = 40 */
/* 4270 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4272 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4274 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 4276 */	0xa,		/* 10 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 4278 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4280 */	NdrFcShort( 0x1 ),	/* 1 */
/* 4282 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4284 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter origin_url */

/* 4286 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 4288 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 4290 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Parameter create_elevated */

/* 4292 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 4294 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 4296 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter status */

/* 4298 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 4300 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 4302 */	NdrFcShort( 0x41a ),	/* Type Offset=1050 */

	/* Return value */

/* 4304 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4306 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 4308 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Send */

/* 4310 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4312 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4316 */	NdrFcShort( 0x3 ),	/* 3 */
/* 4318 */	NdrFcShort( 0x48 ),	/* X64 Stack size/offset = 72 */
/* 4320 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4322 */	NdrFcShort( 0x24 ),	/* 36 */
/* 4324 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x8,		/* 8 */
/* 4326 */	0xa,		/* 10 */
			0x87,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, has big byval param */
/* 4328 */	NdrFcShort( 0x1 ),	/* 1 */
/* 4330 */	NdrFcShort( 0x1 ),	/* 1 */
/* 4332 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4334 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter url */

/* 4336 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 4338 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 4340 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Parameter post_data */

/* 4342 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 4344 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 4346 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Parameter request_headers */

/* 4348 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 4350 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 4352 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Parameter response_headers_needed */

/* 4354 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 4356 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 4358 */	NdrFcShort( 0x40c ),	/* Type Offset=1036 */

	/* Parameter response_headers */

/* 4360 */	NdrFcShort( 0x6113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=24 */
/* 4362 */	NdrFcShort( 0x28 ),	/* X64 Stack size/offset = 40 */
/* 4364 */	NdrFcShort( 0x3fa ),	/* Type Offset=1018 */

	/* Parameter response_code */

/* 4366 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 4368 */	NdrFcShort( 0x30 ),	/* X64 Stack size/offset = 48 */
/* 4370 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter cache_filename */

/* 4372 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 4374 */	NdrFcShort( 0x38 ),	/* X64 Stack size/offset = 56 */
/* 4376 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Return value */

/* 4378 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4380 */	NdrFcShort( 0x40 ),	/* X64 Stack size/offset = 64 */
/* 4382 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LaunchCmdLine */

/* 4384 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4386 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4390 */	NdrFcShort( 0x3 ),	/* 3 */
/* 4392 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 4394 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4396 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4398 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 4400 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 4402 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4404 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4406 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4408 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter cmd_line */

/* 4410 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 4412 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 4414 */	NdrFcShort( 0x432 ),	/* Type Offset=1074 */

	/* Return value */

/* 4416 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4418 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 4420 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LaunchBrowser */

/* 4422 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4424 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4428 */	NdrFcShort( 0x4 ),	/* 4 */
/* 4430 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 4432 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4434 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4436 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 4438 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 4440 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4442 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4444 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4446 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter browser_type */

/* 4448 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 4450 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 4452 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter url */

/* 4454 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 4456 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 4458 */	NdrFcShort( 0x432 ),	/* Type Offset=1074 */

	/* Return value */

/* 4460 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4462 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 4464 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LaunchCmdElevated */

/* 4466 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4468 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4472 */	NdrFcShort( 0x5 ),	/* 5 */
/* 4474 */	NdrFcShort( 0x30 ),	/* X64 Stack size/offset = 48 */
/* 4476 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4478 */	NdrFcShort( 0x24 ),	/* 36 */
/* 4480 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x5,		/* 5 */
/* 4482 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 4484 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4486 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4488 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4490 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter app_guid */

/* 4492 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 4494 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 4496 */	NdrFcShort( 0x432 ),	/* Type Offset=1074 */

	/* Parameter cmd_id */

/* 4498 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 4500 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 4502 */	NdrFcShort( 0x432 ),	/* Type Offset=1074 */

	/* Parameter caller_proc_id */

/* 4504 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 4506 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 4508 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter proc_handle */

/* 4510 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 4512 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 4514 */	0xb9,		/* FC_UINT3264 */
			0x0,		/* 0 */

	/* Return value */

/* 4516 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4518 */	NdrFcShort( 0x28 ),	/* X64 Stack size/offset = 40 */
/* 4520 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LaunchCmdLineEx */

/* 4522 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4524 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4528 */	NdrFcShort( 0x6 ),	/* 6 */
/* 4530 */	NdrFcShort( 0x30 ),	/* X64 Stack size/offset = 48 */
/* 4532 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4534 */	NdrFcShort( 0x5c ),	/* 92 */
/* 4536 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x5,		/* 5 */
/* 4538 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 4540 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4542 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4544 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4546 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter cmd_line */

/* 4548 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 4550 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 4552 */	NdrFcShort( 0x432 ),	/* Type Offset=1074 */

	/* Parameter server_proc_id */

/* 4554 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 4556 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 4558 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter proc_handle */

/* 4560 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 4562 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 4564 */	0xb9,		/* FC_UINT3264 */
			0x0,		/* 0 */

	/* Parameter stdout_handle */

/* 4566 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 4568 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 4570 */	0xb9,		/* FC_UINT3264 */
			0x0,		/* 0 */

	/* Return value */

/* 4572 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4574 */	NdrFcShort( 0x28 ),	/* X64 Stack size/offset = 40 */
/* 4576 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LaunchAppCommand */

/* 4578 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4580 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4584 */	NdrFcShort( 0x3 ),	/* 3 */
/* 4586 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 4588 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4590 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4592 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 4594 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 4596 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4598 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4600 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4602 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter app_guid */

/* 4604 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 4606 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 4608 */	NdrFcShort( 0x432 ),	/* Type Offset=1074 */

	/* Parameter cmd_id */

/* 4610 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 4612 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 4614 */	NdrFcShort( 0x432 ),	/* Type Offset=1074 */

	/* Return value */

/* 4616 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4618 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 4620 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure OnShow */


	/* Procedure DoClose */

/* 4622 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4624 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4628 */	NdrFcShort( 0x3 ),	/* 3 */
/* 4630 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 4632 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4634 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4636 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 4638 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 4640 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4642 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4644 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4646 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */


	/* Return value */

/* 4648 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4650 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 4652 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure OnCheckingForUpdate */


	/* Procedure DoPause */

/* 4654 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4656 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4660 */	NdrFcShort( 0x4 ),	/* 4 */
/* 4662 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 4664 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4666 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4668 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 4670 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 4672 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4674 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4676 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4678 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */


	/* Return value */

/* 4680 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4682 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 4684 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure DoResume */

/* 4686 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4688 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4692 */	NdrFcShort( 0x5 ),	/* 5 */
/* 4694 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 4696 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4698 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4700 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 4702 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 4704 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4706 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4708 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4710 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 4712 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4714 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 4716 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure OnWaitingToDownload */


	/* Procedure DoRestartBrowsers */

/* 4718 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4720 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4724 */	NdrFcShort( 0x6 ),	/* 6 */
/* 4726 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 4728 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4730 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4732 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 4734 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 4736 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4738 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4740 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4742 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */


	/* Return value */

/* 4744 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4746 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 4748 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure DoReboot */

/* 4750 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4752 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4756 */	NdrFcShort( 0x7 ),	/* 7 */
/* 4758 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 4760 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4762 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4764 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 4766 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 4768 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4770 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4772 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4774 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 4776 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4778 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 4780 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure DoLaunchBrowser */

/* 4782 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4784 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4788 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4790 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 4792 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4794 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4796 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 4798 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 4800 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4802 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4804 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4806 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter url */

/* 4808 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 4810 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 4812 */	NdrFcShort( 0x432 ),	/* Type Offset=1074 */

	/* Return value */

/* 4814 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4816 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 4818 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure OnUpdateAvailable */

/* 4820 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4822 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4826 */	NdrFcShort( 0x5 ),	/* 5 */
/* 4828 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 4830 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4832 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4834 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 4836 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 4838 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4840 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4842 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4844 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter version_string */

/* 4846 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 4848 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 4850 */	NdrFcShort( 0x432 ),	/* Type Offset=1074 */

	/* Return value */

/* 4852 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4854 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 4856 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure OnDownloading */

/* 4858 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4860 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4864 */	NdrFcShort( 0x7 ),	/* 7 */
/* 4866 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 4868 */	NdrFcShort( 0x10 ),	/* 16 */
/* 4870 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4872 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x3,		/* 3 */
/* 4874 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 4876 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4878 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4880 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4882 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter time_remaining_ms */

/* 4884 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 4886 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 4888 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pos */

/* 4890 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 4892 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 4894 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 4896 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4898 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 4900 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure OnWaitingToInstall */

/* 4902 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4904 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4908 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4910 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 4912 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4914 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4916 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 4918 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 4920 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4922 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4924 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4926 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 4928 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4930 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 4932 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure OnPause */

/* 4934 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4936 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4940 */	NdrFcShort( 0xa ),	/* 10 */
/* 4942 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 4944 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4946 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4948 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 4950 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 4952 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4954 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4956 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4958 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 4960 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4962 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 4964 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure OnComplete */

/* 4966 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4968 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4972 */	NdrFcShort( 0xb ),	/* 11 */
/* 4974 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 4976 */	NdrFcShort( 0x6 ),	/* 6 */
/* 4978 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4980 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 4982 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 4984 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4986 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4988 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4990 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter code */

/* 4992 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 4994 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 4996 */	0xd,		/* FC_ENUM16 */
			0x0,		/* 0 */

	/* Parameter completion_text */

/* 4998 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 5000 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 5002 */	NdrFcShort( 0x432 ),	/* Type Offset=1074 */

	/* Return value */

/* 5004 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5006 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 5008 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure SetEventSink */

/* 5010 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5012 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5016 */	NdrFcShort( 0xc ),	/* 12 */
/* 5018 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 5020 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5022 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5024 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 5026 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 5028 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5030 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5032 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5034 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ui_sink */

/* 5036 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 5038 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 5040 */	NdrFcShort( 0x438 ),	/* Type Offset=1080 */

	/* Return value */

/* 5042 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5044 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 5046 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure OnInstalling2 */

/* 5048 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5050 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5054 */	NdrFcShort( 0x3 ),	/* 3 */
/* 5056 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 5058 */	NdrFcShort( 0x10 ),	/* 16 */
/* 5060 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5062 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x3,		/* 3 */
/* 5064 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 5066 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5068 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5070 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5072 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter time_remaining_ms */

/* 5074 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 5076 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 5078 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pos */

/* 5080 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 5082 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 5084 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 5086 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5088 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 5090 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure CheckForUpdate */

/* 5092 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5094 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5098 */	NdrFcShort( 0x3 ),	/* 3 */
/* 5100 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 5102 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5104 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5106 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 5108 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 5110 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5112 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5114 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5116 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter guid */

/* 5118 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 5120 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 5122 */	NdrFcShort( 0x432 ),	/* Type Offset=1074 */

	/* Parameter observer */

/* 5124 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 5126 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 5128 */	NdrFcShort( 0x44a ),	/* Type Offset=1098 */

	/* Return value */

/* 5130 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5132 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 5134 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Update */

/* 5136 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5138 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5142 */	NdrFcShort( 0x4 ),	/* 4 */
/* 5144 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 5146 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5148 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5150 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 5152 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 5154 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5156 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5158 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5160 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter guid */

/* 5162 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 5164 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 5166 */	NdrFcShort( 0x432 ),	/* Type Offset=1074 */

	/* Parameter observer */

/* 5168 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 5170 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 5172 */	NdrFcShort( 0x44a ),	/* Type Offset=1098 */

	/* Return value */

/* 5174 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5176 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 5178 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LaunchCmdElevated */

/* 5180 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5182 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5186 */	NdrFcShort( 0x3 ),	/* 3 */
/* 5188 */	NdrFcShort( 0x30 ),	/* X64 Stack size/offset = 48 */
/* 5190 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5192 */	NdrFcShort( 0x24 ),	/* 36 */
/* 5194 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x5,		/* 5 */
/* 5196 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 5198 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5200 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5202 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5204 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter app_guid */

/* 5206 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 5208 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 5210 */	NdrFcShort( 0x432 ),	/* Type Offset=1074 */

	/* Parameter cmd_id */

/* 5212 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 5214 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 5216 */	NdrFcShort( 0x432 ),	/* Type Offset=1074 */

	/* Parameter caller_proc_id */

/* 5218 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 5220 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 5222 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter proc_handle */

/* 5224 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 5226 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 5228 */	0xb9,		/* FC_UINT3264 */
			0x0,		/* 0 */

	/* Return value */

/* 5230 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5232 */	NdrFcShort( 0x28 ),	/* X64 Stack size/offset = 40 */
/* 5234 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

			0x0
        }
    };

static const google_update_idl_MIDL_TYPE_FORMAT_STRING google_update_idl__MIDL_TypeFormatString =
    {
        0,
        {
			NdrFcShort( 0x0 ),	/* 0 */
/*  2 */	
			0x11, 0xc,	/* FC_RP [allocated_on_stack] [simple_pointer] */
/*  4 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/*  6 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/*  8 */	NdrFcShort( 0x2 ),	/* Offset= 2 (10) */
/* 10 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 12 */	NdrFcLong( 0x20400 ),	/* 132096 */
/* 16 */	NdrFcShort( 0x0 ),	/* 0 */
/* 18 */	NdrFcShort( 0x0 ),	/* 0 */
/* 20 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 22 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 24 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 26 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 28 */	
			0x11, 0x4,	/* FC_RP [allocated_on_stack] */
/* 30 */	NdrFcShort( 0x1c ),	/* Offset= 28 (58) */
/* 32 */	
			0x13, 0x0,	/* FC_OP */
/* 34 */	NdrFcShort( 0xe ),	/* Offset= 14 (48) */
/* 36 */	
			0x1b,		/* FC_CARRAY */
			0x1,		/* 1 */
/* 38 */	NdrFcShort( 0x2 ),	/* 2 */
/* 40 */	0x9,		/* Corr desc: FC_ULONG */
			0x0,		/*  */
/* 42 */	NdrFcShort( 0xfffc ),	/* -4 */
/* 44 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 46 */	0x6,		/* FC_SHORT */
			0x5b,		/* FC_END */
/* 48 */	
			0x17,		/* FC_CSTRUCT */
			0x3,		/* 3 */
/* 50 */	NdrFcShort( 0x8 ),	/* 8 */
/* 52 */	NdrFcShort( 0xfff0 ),	/* Offset= -16 (36) */
/* 54 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 56 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 58 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 60 */	NdrFcShort( 0x0 ),	/* 0 */
/* 62 */	NdrFcShort( 0x8 ),	/* 8 */
/* 64 */	NdrFcShort( 0x0 ),	/* 0 */
/* 66 */	NdrFcShort( 0xffde ),	/* Offset= -34 (32) */
/* 68 */	
			0x12, 0x0,	/* FC_UP */
/* 70 */	NdrFcShort( 0xffea ),	/* Offset= -22 (48) */
/* 72 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 74 */	NdrFcShort( 0x0 ),	/* 0 */
/* 76 */	NdrFcShort( 0x8 ),	/* 8 */
/* 78 */	NdrFcShort( 0x0 ),	/* 0 */
/* 80 */	NdrFcShort( 0xfff4 ),	/* Offset= -12 (68) */
/* 82 */	
			0x11, 0xc,	/* FC_RP [allocated_on_stack] [simple_pointer] */
/* 84 */	0x6,		/* FC_SHORT */
			0x5c,		/* FC_PAD */
/* 86 */	
			0x11, 0x4,	/* FC_RP [allocated_on_stack] */
/* 88 */	NdrFcShort( 0x3a2 ),	/* Offset= 930 (1018) */
/* 90 */	
			0x13, 0x0,	/* FC_OP */
/* 92 */	NdrFcShort( 0x38a ),	/* Offset= 906 (998) */
/* 94 */	
			0x2b,		/* FC_NON_ENCAPSULATED_UNION */
			0x9,		/* FC_ULONG */
/* 96 */	0x7,		/* Corr desc: FC_USHORT */
			0x0,		/*  */
/* 98 */	NdrFcShort( 0xfff8 ),	/* -8 */
/* 100 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 102 */	NdrFcShort( 0x2 ),	/* Offset= 2 (104) */
/* 104 */	NdrFcShort( 0x10 ),	/* 16 */
/* 106 */	NdrFcShort( 0x2f ),	/* 47 */
/* 108 */	NdrFcLong( 0x14 ),	/* 20 */
/* 112 */	NdrFcShort( 0x800b ),	/* Simple arm type: FC_HYPER */
/* 114 */	NdrFcLong( 0x3 ),	/* 3 */
/* 118 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 120 */	NdrFcLong( 0x11 ),	/* 17 */
/* 124 */	NdrFcShort( 0x8001 ),	/* Simple arm type: FC_BYTE */
/* 126 */	NdrFcLong( 0x2 ),	/* 2 */
/* 130 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 132 */	NdrFcLong( 0x4 ),	/* 4 */
/* 136 */	NdrFcShort( 0x800a ),	/* Simple arm type: FC_FLOAT */
/* 138 */	NdrFcLong( 0x5 ),	/* 5 */
/* 142 */	NdrFcShort( 0x800c ),	/* Simple arm type: FC_DOUBLE */
/* 144 */	NdrFcLong( 0xb ),	/* 11 */
/* 148 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 150 */	NdrFcLong( 0xa ),	/* 10 */
/* 154 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 156 */	NdrFcLong( 0x6 ),	/* 6 */
/* 160 */	NdrFcShort( 0xe8 ),	/* Offset= 232 (392) */
/* 162 */	NdrFcLong( 0x7 ),	/* 7 */
/* 166 */	NdrFcShort( 0x800c ),	/* Simple arm type: FC_DOUBLE */
/* 168 */	NdrFcLong( 0x8 ),	/* 8 */
/* 172 */	NdrFcShort( 0xff74 ),	/* Offset= -140 (32) */
/* 174 */	NdrFcLong( 0xd ),	/* 13 */
/* 178 */	NdrFcShort( 0xdc ),	/* Offset= 220 (398) */
/* 180 */	NdrFcLong( 0x9 ),	/* 9 */
/* 184 */	NdrFcShort( 0xff52 ),	/* Offset= -174 (10) */
/* 186 */	NdrFcLong( 0x2000 ),	/* 8192 */
/* 190 */	NdrFcShort( 0xe2 ),	/* Offset= 226 (416) */
/* 192 */	NdrFcLong( 0x24 ),	/* 36 */
/* 196 */	NdrFcShort( 0x2d8 ),	/* Offset= 728 (924) */
/* 198 */	NdrFcLong( 0x4024 ),	/* 16420 */
/* 202 */	NdrFcShort( 0x2d2 ),	/* Offset= 722 (924) */
/* 204 */	NdrFcLong( 0x4011 ),	/* 16401 */
/* 208 */	NdrFcShort( 0x2d0 ),	/* Offset= 720 (928) */
/* 210 */	NdrFcLong( 0x4002 ),	/* 16386 */
/* 214 */	NdrFcShort( 0x2ce ),	/* Offset= 718 (932) */
/* 216 */	NdrFcLong( 0x4003 ),	/* 16387 */
/* 220 */	NdrFcShort( 0x2cc ),	/* Offset= 716 (936) */
/* 222 */	NdrFcLong( 0x4014 ),	/* 16404 */
/* 226 */	NdrFcShort( 0x2ca ),	/* Offset= 714 (940) */
/* 228 */	NdrFcLong( 0x4004 ),	/* 16388 */
/* 232 */	NdrFcShort( 0x2c8 ),	/* Offset= 712 (944) */
/* 234 */	NdrFcLong( 0x4005 ),	/* 16389 */
/* 238 */	NdrFcShort( 0x2c6 ),	/* Offset= 710 (948) */
/* 240 */	NdrFcLong( 0x400b ),	/* 16395 */
/* 244 */	NdrFcShort( 0x2b0 ),	/* Offset= 688 (932) */
/* 246 */	NdrFcLong( 0x400a ),	/* 16394 */
/* 250 */	NdrFcShort( 0x2ae ),	/* Offset= 686 (936) */
/* 252 */	NdrFcLong( 0x4006 ),	/* 16390 */
/* 256 */	NdrFcShort( 0x2b8 ),	/* Offset= 696 (952) */
/* 258 */	NdrFcLong( 0x4007 ),	/* 16391 */
/* 262 */	NdrFcShort( 0x2ae ),	/* Offset= 686 (948) */
/* 264 */	NdrFcLong( 0x4008 ),	/* 16392 */
/* 268 */	NdrFcShort( 0x2b0 ),	/* Offset= 688 (956) */
/* 270 */	NdrFcLong( 0x400d ),	/* 16397 */
/* 274 */	NdrFcShort( 0x2ae ),	/* Offset= 686 (960) */
/* 276 */	NdrFcLong( 0x4009 ),	/* 16393 */
/* 280 */	NdrFcShort( 0x2ac ),	/* Offset= 684 (964) */
/* 282 */	NdrFcLong( 0x6000 ),	/* 24576 */
/* 286 */	NdrFcShort( 0x2aa ),	/* Offset= 682 (968) */
/* 288 */	NdrFcLong( 0x400c ),	/* 16396 */
/* 292 */	NdrFcShort( 0x2a8 ),	/* Offset= 680 (972) */
/* 294 */	NdrFcLong( 0x10 ),	/* 16 */
/* 298 */	NdrFcShort( 0x8002 ),	/* Simple arm type: FC_CHAR */
/* 300 */	NdrFcLong( 0x12 ),	/* 18 */
/* 304 */	NdrFcShort( 0x8006 ),	/* Simple arm type: FC_SHORT */
/* 306 */	NdrFcLong( 0x13 ),	/* 19 */
/* 310 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 312 */	NdrFcLong( 0x15 ),	/* 21 */
/* 316 */	NdrFcShort( 0x800b ),	/* Simple arm type: FC_HYPER */
/* 318 */	NdrFcLong( 0x16 ),	/* 22 */
/* 322 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 324 */	NdrFcLong( 0x17 ),	/* 23 */
/* 328 */	NdrFcShort( 0x8008 ),	/* Simple arm type: FC_LONG */
/* 330 */	NdrFcLong( 0xe ),	/* 14 */
/* 334 */	NdrFcShort( 0x286 ),	/* Offset= 646 (980) */
/* 336 */	NdrFcLong( 0x400e ),	/* 16398 */
/* 340 */	NdrFcShort( 0x28a ),	/* Offset= 650 (990) */
/* 342 */	NdrFcLong( 0x4010 ),	/* 16400 */
/* 346 */	NdrFcShort( 0x288 ),	/* Offset= 648 (994) */
/* 348 */	NdrFcLong( 0x4012 ),	/* 16402 */
/* 352 */	NdrFcShort( 0x244 ),	/* Offset= 580 (932) */
/* 354 */	NdrFcLong( 0x4013 ),	/* 16403 */
/* 358 */	NdrFcShort( 0x242 ),	/* Offset= 578 (936) */
/* 360 */	NdrFcLong( 0x4015 ),	/* 16405 */
/* 364 */	NdrFcShort( 0x240 ),	/* Offset= 576 (940) */
/* 366 */	NdrFcLong( 0x4016 ),	/* 16406 */
/* 370 */	NdrFcShort( 0x236 ),	/* Offset= 566 (936) */
/* 372 */	NdrFcLong( 0x4017 ),	/* 16407 */
/* 376 */	NdrFcShort( 0x230 ),	/* Offset= 560 (936) */
/* 378 */	NdrFcLong( 0x0 ),	/* 0 */
/* 382 */	NdrFcShort( 0x0 ),	/* Offset= 0 (382) */
/* 384 */	NdrFcLong( 0x1 ),	/* 1 */
/* 388 */	NdrFcShort( 0x0 ),	/* Offset= 0 (388) */
/* 390 */	NdrFcShort( 0xffff ),	/* Offset= -1 (389) */
/* 392 */	
			0x15,		/* FC_STRUCT */
			0x7,		/* 7 */
/* 394 */	NdrFcShort( 0x8 ),	/* 8 */
/* 396 */	0xb,		/* FC_HYPER */
			0x5b,		/* FC_END */
/* 398 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 400 */	NdrFcLong( 0x0 ),	/* 0 */
/* 404 */	NdrFcShort( 0x0 ),	/* 0 */
/* 406 */	NdrFcShort( 0x0 ),	/* 0 */
/* 408 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 410 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 412 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 414 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 416 */	
			0x13, 0x10,	/* FC_OP [pointer_deref] */
/* 418 */	NdrFcShort( 0x2 ),	/* Offset= 2 (420) */
/* 420 */	
			0x13, 0x0,	/* FC_OP */
/* 422 */	NdrFcShort( 0x1e4 ),	/* Offset= 484 (906) */
/* 424 */	
			0x2a,		/* FC_ENCAPSULATED_UNION */
			0x89,		/* 137 */
/* 426 */	NdrFcShort( 0x20 ),	/* 32 */
/* 428 */	NdrFcShort( 0xa ),	/* 10 */
/* 430 */	NdrFcLong( 0x8 ),	/* 8 */
/* 434 */	NdrFcShort( 0x50 ),	/* Offset= 80 (514) */
/* 436 */	NdrFcLong( 0xd ),	/* 13 */
/* 440 */	NdrFcShort( 0x70 ),	/* Offset= 112 (552) */
/* 442 */	NdrFcLong( 0x9 ),	/* 9 */
/* 446 */	NdrFcShort( 0x90 ),	/* Offset= 144 (590) */
/* 448 */	NdrFcLong( 0xc ),	/* 12 */
/* 452 */	NdrFcShort( 0xb0 ),	/* Offset= 176 (628) */
/* 454 */	NdrFcLong( 0x24 ),	/* 36 */
/* 458 */	NdrFcShort( 0x102 ),	/* Offset= 258 (716) */
/* 460 */	NdrFcLong( 0x800d ),	/* 32781 */
/* 464 */	NdrFcShort( 0x11e ),	/* Offset= 286 (750) */
/* 466 */	NdrFcLong( 0x10 ),	/* 16 */
/* 470 */	NdrFcShort( 0x138 ),	/* Offset= 312 (782) */
/* 472 */	NdrFcLong( 0x2 ),	/* 2 */
/* 476 */	NdrFcShort( 0x14e ),	/* Offset= 334 (810) */
/* 478 */	NdrFcLong( 0x3 ),	/* 3 */
/* 482 */	NdrFcShort( 0x164 ),	/* Offset= 356 (838) */
/* 484 */	NdrFcLong( 0x14 ),	/* 20 */
/* 488 */	NdrFcShort( 0x17a ),	/* Offset= 378 (866) */
/* 490 */	NdrFcShort( 0xffff ),	/* Offset= -1 (489) */
/* 492 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 494 */	NdrFcShort( 0x0 ),	/* 0 */
/* 496 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 498 */	NdrFcShort( 0x0 ),	/* 0 */
/* 500 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 502 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 506 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 508 */	
			0x13, 0x0,	/* FC_OP */
/* 510 */	NdrFcShort( 0xfe32 ),	/* Offset= -462 (48) */
/* 512 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 514 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 516 */	NdrFcShort( 0x10 ),	/* 16 */
/* 518 */	NdrFcShort( 0x0 ),	/* 0 */
/* 520 */	NdrFcShort( 0x6 ),	/* Offset= 6 (526) */
/* 522 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 524 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 526 */	
			0x11, 0x0,	/* FC_RP */
/* 528 */	NdrFcShort( 0xffdc ),	/* Offset= -36 (492) */
/* 530 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 532 */	NdrFcShort( 0x0 ),	/* 0 */
/* 534 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 536 */	NdrFcShort( 0x0 ),	/* 0 */
/* 538 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 540 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 544 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 546 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 548 */	NdrFcShort( 0xff6a ),	/* Offset= -150 (398) */
/* 550 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 552 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 554 */	NdrFcShort( 0x10 ),	/* 16 */
/* 556 */	NdrFcShort( 0x0 ),	/* 0 */
/* 558 */	NdrFcShort( 0x6 ),	/* Offset= 6 (564) */
/* 560 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 562 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 564 */	
			0x11, 0x0,	/* FC_RP */
/* 566 */	NdrFcShort( 0xffdc ),	/* Offset= -36 (530) */
/* 568 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 570 */	NdrFcShort( 0x0 ),	/* 0 */
/* 572 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 574 */	NdrFcShort( 0x0 ),	/* 0 */
/* 576 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 578 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 582 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 584 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 586 */	NdrFcShort( 0xfdc0 ),	/* Offset= -576 (10) */
/* 588 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 590 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 592 */	NdrFcShort( 0x10 ),	/* 16 */
/* 594 */	NdrFcShort( 0x0 ),	/* 0 */
/* 596 */	NdrFcShort( 0x6 ),	/* Offset= 6 (602) */
/* 598 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 600 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 602 */	
			0x11, 0x0,	/* FC_RP */
/* 604 */	NdrFcShort( 0xffdc ),	/* Offset= -36 (568) */
/* 606 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 608 */	NdrFcShort( 0x0 ),	/* 0 */
/* 610 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 612 */	NdrFcShort( 0x0 ),	/* 0 */
/* 614 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 616 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 620 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 622 */	
			0x13, 0x0,	/* FC_OP */
/* 624 */	NdrFcShort( 0x176 ),	/* Offset= 374 (998) */
/* 626 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 628 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 630 */	NdrFcShort( 0x10 ),	/* 16 */
/* 632 */	NdrFcShort( 0x0 ),	/* 0 */
/* 634 */	NdrFcShort( 0x6 ),	/* Offset= 6 (640) */
/* 636 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 638 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 640 */	
			0x11, 0x0,	/* FC_RP */
/* 642 */	NdrFcShort( 0xffdc ),	/* Offset= -36 (606) */
/* 644 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 646 */	NdrFcLong( 0x2f ),	/* 47 */
/* 650 */	NdrFcShort( 0x0 ),	/* 0 */
/* 652 */	NdrFcShort( 0x0 ),	/* 0 */
/* 654 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 656 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 658 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 660 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 662 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 664 */	NdrFcShort( 0x1 ),	/* 1 */
/* 666 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 668 */	NdrFcShort( 0x4 ),	/* 4 */
/* 670 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 672 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 674 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 676 */	NdrFcShort( 0x18 ),	/* 24 */
/* 678 */	NdrFcShort( 0x0 ),	/* 0 */
/* 680 */	NdrFcShort( 0xa ),	/* Offset= 10 (690) */
/* 682 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 684 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 686 */	NdrFcShort( 0xffd6 ),	/* Offset= -42 (644) */
/* 688 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 690 */	
			0x13, 0x0,	/* FC_OP */
/* 692 */	NdrFcShort( 0xffe2 ),	/* Offset= -30 (662) */
/* 694 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 696 */	NdrFcShort( 0x0 ),	/* 0 */
/* 698 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 700 */	NdrFcShort( 0x0 ),	/* 0 */
/* 702 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 704 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 708 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 710 */	
			0x13, 0x0,	/* FC_OP */
/* 712 */	NdrFcShort( 0xffda ),	/* Offset= -38 (674) */
/* 714 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 716 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 718 */	NdrFcShort( 0x10 ),	/* 16 */
/* 720 */	NdrFcShort( 0x0 ),	/* 0 */
/* 722 */	NdrFcShort( 0x6 ),	/* Offset= 6 (728) */
/* 724 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 726 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 728 */	
			0x11, 0x0,	/* FC_RP */
/* 730 */	NdrFcShort( 0xffdc ),	/* Offset= -36 (694) */
/* 732 */	
			0x1d,		/* FC_SMFARRAY */
			0x0,		/* 0 */
/* 734 */	NdrFcShort( 0x8 ),	/* 8 */
/* 736 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 738 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 740 */	NdrFcShort( 0x10 ),	/* 16 */
/* 742 */	0x8,		/* FC_LONG */
			0x6,		/* FC_SHORT */
/* 744 */	0x6,		/* FC_SHORT */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 746 */	0x0,		/* 0 */
			NdrFcShort( 0xfff1 ),	/* Offset= -15 (732) */
			0x5b,		/* FC_END */
/* 750 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 752 */	NdrFcShort( 0x20 ),	/* 32 */
/* 754 */	NdrFcShort( 0x0 ),	/* 0 */
/* 756 */	NdrFcShort( 0xa ),	/* Offset= 10 (766) */
/* 758 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 760 */	0x36,		/* FC_POINTER */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 762 */	0x0,		/* 0 */
			NdrFcShort( 0xffe7 ),	/* Offset= -25 (738) */
			0x5b,		/* FC_END */
/* 766 */	
			0x11, 0x0,	/* FC_RP */
/* 768 */	NdrFcShort( 0xff12 ),	/* Offset= -238 (530) */
/* 770 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 772 */	NdrFcShort( 0x1 ),	/* 1 */
/* 774 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 776 */	NdrFcShort( 0x0 ),	/* 0 */
/* 778 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 780 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 782 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 784 */	NdrFcShort( 0x10 ),	/* 16 */
/* 786 */	NdrFcShort( 0x0 ),	/* 0 */
/* 788 */	NdrFcShort( 0x6 ),	/* Offset= 6 (794) */
/* 790 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 792 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 794 */	
			0x13, 0x0,	/* FC_OP */
/* 796 */	NdrFcShort( 0xffe6 ),	/* Offset= -26 (770) */
/* 798 */	
			0x1b,		/* FC_CARRAY */
			0x1,		/* 1 */
/* 800 */	NdrFcShort( 0x2 ),	/* 2 */
/* 802 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 804 */	NdrFcShort( 0x0 ),	/* 0 */
/* 806 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 808 */	0x6,		/* FC_SHORT */
			0x5b,		/* FC_END */
/* 810 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 812 */	NdrFcShort( 0x10 ),	/* 16 */
/* 814 */	NdrFcShort( 0x0 ),	/* 0 */
/* 816 */	NdrFcShort( 0x6 ),	/* Offset= 6 (822) */
/* 818 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 820 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 822 */	
			0x13, 0x0,	/* FC_OP */
/* 824 */	NdrFcShort( 0xffe6 ),	/* Offset= -26 (798) */
/* 826 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 828 */	NdrFcShort( 0x4 ),	/* 4 */
/* 830 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 832 */	NdrFcShort( 0x0 ),	/* 0 */
/* 834 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 836 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 838 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 840 */	NdrFcShort( 0x10 ),	/* 16 */
/* 842 */	NdrFcShort( 0x0 ),	/* 0 */
/* 844 */	NdrFcShort( 0x6 ),	/* Offset= 6 (850) */
/* 846 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 848 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 850 */	
			0x13, 0x0,	/* FC_OP */
/* 852 */	NdrFcShort( 0xffe6 ),	/* Offset= -26 (826) */
/* 854 */	
			0x1b,		/* FC_CARRAY */
			0x7,		/* 7 */
/* 856 */	NdrFcShort( 0x8 ),	/* 8 */
/* 858 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 860 */	NdrFcShort( 0x0 ),	/* 0 */
/* 862 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 864 */	0xb,		/* FC_HYPER */
			0x5b,		/* FC_END */
/* 866 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 868 */	NdrFcShort( 0x10 ),	/* 16 */
/* 870 */	NdrFcShort( 0x0 ),	/* 0 */
/* 872 */	NdrFcShort( 0x6 ),	/* Offset= 6 (878) */
/* 874 */	0x8,		/* FC_LONG */
			0x40,		/* FC_STRUCTPAD4 */
/* 876 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 878 */	
			0x13, 0x0,	/* FC_OP */
/* 880 */	NdrFcShort( 0xffe6 ),	/* Offset= -26 (854) */
/* 882 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 884 */	NdrFcShort( 0x8 ),	/* 8 */
/* 886 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 888 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 890 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 892 */	NdrFcShort( 0x8 ),	/* 8 */
/* 894 */	0x7,		/* Corr desc: FC_USHORT */
			0x0,		/*  */
/* 896 */	NdrFcShort( 0xffc8 ),	/* -56 */
/* 898 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 900 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 902 */	NdrFcShort( 0xffec ),	/* Offset= -20 (882) */
/* 904 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 906 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 908 */	NdrFcShort( 0x38 ),	/* 56 */
/* 910 */	NdrFcShort( 0xffec ),	/* Offset= -20 (890) */
/* 912 */	NdrFcShort( 0x0 ),	/* Offset= 0 (912) */
/* 914 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 916 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 918 */	0x40,		/* FC_STRUCTPAD4 */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 920 */	0x0,		/* 0 */
			NdrFcShort( 0xfe0f ),	/* Offset= -497 (424) */
			0x5b,		/* FC_END */
/* 924 */	
			0x13, 0x0,	/* FC_OP */
/* 926 */	NdrFcShort( 0xff04 ),	/* Offset= -252 (674) */
/* 928 */	
			0x13, 0x8,	/* FC_OP [simple_pointer] */
/* 930 */	0x1,		/* FC_BYTE */
			0x5c,		/* FC_PAD */
/* 932 */	
			0x13, 0x8,	/* FC_OP [simple_pointer] */
/* 934 */	0x6,		/* FC_SHORT */
			0x5c,		/* FC_PAD */
/* 936 */	
			0x13, 0x8,	/* FC_OP [simple_pointer] */
/* 938 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 940 */	
			0x13, 0x8,	/* FC_OP [simple_pointer] */
/* 942 */	0xb,		/* FC_HYPER */
			0x5c,		/* FC_PAD */
/* 944 */	
			0x13, 0x8,	/* FC_OP [simple_pointer] */
/* 946 */	0xa,		/* FC_FLOAT */
			0x5c,		/* FC_PAD */
/* 948 */	
			0x13, 0x8,	/* FC_OP [simple_pointer] */
/* 950 */	0xc,		/* FC_DOUBLE */
			0x5c,		/* FC_PAD */
/* 952 */	
			0x13, 0x0,	/* FC_OP */
/* 954 */	NdrFcShort( 0xfdce ),	/* Offset= -562 (392) */
/* 956 */	
			0x13, 0x10,	/* FC_OP [pointer_deref] */
/* 958 */	NdrFcShort( 0xfc62 ),	/* Offset= -926 (32) */
/* 960 */	
			0x13, 0x10,	/* FC_OP [pointer_deref] */
/* 962 */	NdrFcShort( 0xfdcc ),	/* Offset= -564 (398) */
/* 964 */	
			0x13, 0x10,	/* FC_OP [pointer_deref] */
/* 966 */	NdrFcShort( 0xfc44 ),	/* Offset= -956 (10) */
/* 968 */	
			0x13, 0x10,	/* FC_OP [pointer_deref] */
/* 970 */	NdrFcShort( 0xfdd6 ),	/* Offset= -554 (416) */
/* 972 */	
			0x13, 0x10,	/* FC_OP [pointer_deref] */
/* 974 */	NdrFcShort( 0x2 ),	/* Offset= 2 (976) */
/* 976 */	
			0x13, 0x0,	/* FC_OP */
/* 978 */	NdrFcShort( 0x14 ),	/* Offset= 20 (998) */
/* 980 */	
			0x15,		/* FC_STRUCT */
			0x7,		/* 7 */
/* 982 */	NdrFcShort( 0x10 ),	/* 16 */
/* 984 */	0x6,		/* FC_SHORT */
			0x1,		/* FC_BYTE */
/* 986 */	0x1,		/* FC_BYTE */
			0x8,		/* FC_LONG */
/* 988 */	0xb,		/* FC_HYPER */
			0x5b,		/* FC_END */
/* 990 */	
			0x13, 0x0,	/* FC_OP */
/* 992 */	NdrFcShort( 0xfff4 ),	/* Offset= -12 (980) */
/* 994 */	
			0x13, 0x8,	/* FC_OP [simple_pointer] */
/* 996 */	0x2,		/* FC_CHAR */
			0x5c,		/* FC_PAD */
/* 998 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x7,		/* 7 */
/* 1000 */	NdrFcShort( 0x20 ),	/* 32 */
/* 1002 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1004 */	NdrFcShort( 0x0 ),	/* Offset= 0 (1004) */
/* 1006 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 1008 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 1010 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 1012 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1014 */	NdrFcShort( 0xfc68 ),	/* Offset= -920 (94) */
/* 1016 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1018 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 1020 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1022 */	NdrFcShort( 0x18 ),	/* 24 */
/* 1024 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1026 */	NdrFcShort( 0xfc58 ),	/* Offset= -936 (90) */
/* 1028 */	
			0x11, 0x0,	/* FC_RP */
/* 1030 */	NdrFcShort( 0x6 ),	/* Offset= 6 (1036) */
/* 1032 */	
			0x12, 0x0,	/* FC_UP */
/* 1034 */	NdrFcShort( 0xffdc ),	/* Offset= -36 (998) */
/* 1036 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 1038 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1040 */	NdrFcShort( 0x18 ),	/* 24 */
/* 1042 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1044 */	NdrFcShort( 0xfff4 ),	/* Offset= -12 (1032) */
/* 1046 */	
			0x11, 0xc,	/* FC_RP [allocated_on_stack] [simple_pointer] */
/* 1048 */	0xb,		/* FC_HYPER */
			0x5c,		/* FC_PAD */
/* 1050 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 1052 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1054) */
/* 1054 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1056 */	NdrFcLong( 0x7a24060e ),	/* 2049181198 */
/* 1060 */	NdrFcShort( 0x533f ),	/* 21311 */
/* 1062 */	NdrFcShort( 0x4962 ),	/* 18786 */
/* 1064 */	0x9e,		/* 158 */
			0x15,		/* 21 */
/* 1066 */	0x34,		/* 52 */
			0xbd,		/* 189 */
/* 1068 */	0x82,		/* 130 */
			0x55,		/* 85 */
/* 1070 */	0x5f,		/* 95 */
			0xa7,		/* 167 */
/* 1072 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 1074 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 1076 */	
			0x11, 0xc,	/* FC_RP [allocated_on_stack] [simple_pointer] */
/* 1078 */	0xb9,		/* FC_UINT3264 */
			0x5c,		/* FC_PAD */
/* 1080 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1082 */	NdrFcLong( 0x8504fb26 ),	/* -2063271130 */
/* 1086 */	NdrFcShort( 0xfc3e ),	/* -962 */
/* 1088 */	NdrFcShort( 0x4c1c ),	/* 19484 */
/* 1090 */	0x9c,		/* 156 */
			0x94,		/* 148 */
/* 1092 */	0x46,		/* 70 */
			0xec,		/* 236 */
/* 1094 */	0x93,		/* 147 */
			0xe6,		/* 230 */
/* 1096 */	0xba,		/* 186 */
			0x63,		/* 99 */
/* 1098 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1100 */	NdrFcLong( 0x7cfc4e00 ),	/* 2096909824 */
/* 1104 */	NdrFcShort( 0x1c9d ),	/* 7325 */
/* 1106 */	NdrFcShort( 0x443d ),	/* 17469 */
/* 1108 */	0xb5,		/* 181 */
			0xbe,		/* 190 */
/* 1110 */	0xce,		/* 206 */
			0xee,		/* 238 */
/* 1112 */	0xac,		/* 172 */
			0x14,		/* 20 */
/* 1114 */	0x43,		/* 67 */
			0xaf,		/* 175 */

			0x0
        }
    };

static const USER_MARSHAL_ROUTINE_QUADRUPLE UserMarshalRoutines[ WIRE_MARSHAL_TABLE_SIZE ] = 
        {
            
            {
            BSTR_UserSize
            ,BSTR_UserMarshal
            ,BSTR_UserUnmarshal
            ,BSTR_UserFree
            },
            {
            VARIANT_UserSize
            ,VARIANT_UserMarshal
            ,VARIANT_UserUnmarshal
            ,VARIANT_UserFree
            }

        };



/* Standard interface: __MIDL_itf_google_update_idl_0000_0000, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: IUnknown, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: IDispatch, ver. 0.0,
   GUID={0x00020400,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: IGoogleUpdate3, ver. 0.0,
   GUID={0x7CB305B1,0x4D45,0x4668,{0xAD,0x91,0x67,0x7F,0x87,0xBE,0xD3,0x05}} */

#pragma code_seg(".orpc")
static const unsigned short IGoogleUpdate3_FormatStringOffsetTable[] =
    {
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    0,
    38,
    82
    };

static const MIDL_STUBLESS_PROXY_INFO IGoogleUpdate3_ProxyInfo =
    {
    &Object_StubDesc,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IGoogleUpdate3_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IGoogleUpdate3_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IGoogleUpdate3_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(10) _IGoogleUpdate3ProxyVtbl = 
{
    &IGoogleUpdate3_ProxyInfo,
    &IID_IGoogleUpdate3,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    0 /* IDispatch::GetTypeInfoCount */ ,
    0 /* IDispatch::GetTypeInfo */ ,
    0 /* IDispatch::GetIDsOfNames */ ,
    0 /* IDispatch_Invoke_Proxy */ ,
    (void *) (INT_PTR) -1 /* IGoogleUpdate3::get_Count */ ,
    (void *) (INT_PTR) -1 /* IGoogleUpdate3::get_Item */ ,
    (void *) (INT_PTR) -1 /* IGoogleUpdate3::createAppBundle */
};


static const PRPC_STUB_FUNCTION IGoogleUpdate3_table[] =
{
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2
};

CInterfaceStubVtbl _IGoogleUpdate3StubVtbl =
{
    &IID_IGoogleUpdate3,
    &IGoogleUpdate3_ServerInfo,
    10,
    &IGoogleUpdate3_table[-3],
    CStdStubBuffer_DELEGATING_METHODS
};


/* Object interface: IAppBundle, ver. 0.0,
   GUID={0x1985533F,0x9B0F,0x490A,{0x85,0xC5,0x24,0xF3,0x16,0xE6,0x6F,0xB2}} */

#pragma code_seg(".orpc")
static const unsigned short IAppBundle_FormatStringOffsetTable[] =
    {
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    120,
    158,
    196,
    234,
    272,
    310,
    348,
    386,
    424,
    462,
    500,
    538,
    576,
    614,
    652,
    690,
    728,
    766,
    810,
    860,
    898,
    930,
    974,
    1018,
    1050,
    1082,
    1114,
    1146,
    1178,
    1210,
    1242,
    1274,
    1312,
    1356
    };

static const MIDL_STUBLESS_PROXY_INFO IAppBundle_ProxyInfo =
    {
    &Object_StubDesc,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IAppBundle_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IAppBundle_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IAppBundle_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(41) _IAppBundleProxyVtbl = 
{
    &IAppBundle_ProxyInfo,
    &IID_IAppBundle,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    0 /* IDispatch::GetTypeInfoCount */ ,
    0 /* IDispatch::GetTypeInfo */ ,
    0 /* IDispatch::GetIDsOfNames */ ,
    0 /* IDispatch_Invoke_Proxy */ ,
    (void *) (INT_PTR) -1 /* IAppBundle::get_displayName */ ,
    (void *) (INT_PTR) -1 /* IAppBundle::put_displayName */ ,
    (void *) (INT_PTR) -1 /* IAppBundle::get_displayLanguage */ ,
    (void *) (INT_PTR) -1 /* IAppBundle::put_displayLanguage */ ,
    (void *) (INT_PTR) -1 /* IAppBundle::get_installSource */ ,
    (void *) (INT_PTR) -1 /* IAppBundle::put_installSource */ ,
    (void *) (INT_PTR) -1 /* IAppBundle::get_originURL */ ,
    (void *) (INT_PTR) -1 /* IAppBundle::put_originURL */ ,
    (void *) (INT_PTR) -1 /* IAppBundle::get_offlineDirectory */ ,
    (void *) (INT_PTR) -1 /* IAppBundle::put_offlineDirectory */ ,
    (void *) (INT_PTR) -1 /* IAppBundle::get_sessionId */ ,
    (void *) (INT_PTR) -1 /* IAppBundle::put_sessionId */ ,
    (void *) (INT_PTR) -1 /* IAppBundle::get_sendPings */ ,
    (void *) (INT_PTR) -1 /* IAppBundle::put_sendPings */ ,
    (void *) (INT_PTR) -1 /* IAppBundle::get_priority */ ,
    (void *) (INT_PTR) -1 /* IAppBundle::put_priority */ ,
    (void *) (INT_PTR) -1 /* IAppBundle::get_Count */ ,
    (void *) (INT_PTR) -1 /* IAppBundle::get_Item */ ,
    (void *) (INT_PTR) -1 /* IAppBundle::put_altTokens */ ,
    (void *) (INT_PTR) -1 /* IAppBundle::put_parentHWND */ ,
    (void *) (INT_PTR) -1 /* IAppBundle::initialize */ ,
    (void *) (INT_PTR) -1 /* IAppBundle::createApp */ ,
    (void *) (INT_PTR) -1 /* IAppBundle::createInstalledApp */ ,
    (void *) (INT_PTR) -1 /* IAppBundle::createAllInstalledApps */ ,
    (void *) (INT_PTR) -1 /* IAppBundle::checkForUpdate */ ,
    (void *) (INT_PTR) -1 /* IAppBundle::download */ ,
    (void *) (INT_PTR) -1 /* IAppBundle::install */ ,
    (void *) (INT_PTR) -1 /* IAppBundle::updateAllApps */ ,
    (void *) (INT_PTR) -1 /* IAppBundle::stop */ ,
    (void *) (INT_PTR) -1 /* IAppBundle::pause */ ,
    (void *) (INT_PTR) -1 /* IAppBundle::resume */ ,
    (void *) (INT_PTR) -1 /* IAppBundle::isBusy */ ,
    (void *) (INT_PTR) -1 /* IAppBundle::downloadPackage */ ,
    (void *) (INT_PTR) -1 /* IAppBundle::get_currentState */
};


static const PRPC_STUB_FUNCTION IAppBundle_table[] =
{
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2
};

CInterfaceStubVtbl _IAppBundleStubVtbl =
{
    &IID_IAppBundle,
    &IAppBundle_ServerInfo,
    41,
    &IAppBundle_table[-3],
    CStdStubBuffer_DELEGATING_METHODS
};


/* Object interface: IApp, ver. 0.0,
   GUID={0xA4BCDF52,0x2179,0x4C77,{0x8C,0x5F,0xB8,0x09,0x57,0x12,0xB5,0x63}} */

#pragma code_seg(".orpc")
static const unsigned short IApp_FormatStringOffsetTable[] =
    {
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    1394,
    1432,
    196,
    1470,
    1508,
    1546,
    1584,
    1622,
    1660,
    1698,
    1736,
    1774,
    1812,
    1850,
    1888,
    1926,
    1964,
    2002,
    2040,
    2078,
    2116,
    930,
    2154,
    2192,
    2230,
    2268,
    2306,
    2344,
    2382,
    2420,
    2458,
    2496,
    2534,
    2572
    };

static const MIDL_STUBLESS_PROXY_INFO IApp_ProxyInfo =
    {
    &Object_StubDesc,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IApp_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IApp_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IApp_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(41) _IAppProxyVtbl = 
{
    &IApp_ProxyInfo,
    &IID_IApp,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    0 /* IDispatch::GetTypeInfoCount */ ,
    0 /* IDispatch::GetTypeInfo */ ,
    0 /* IDispatch::GetIDsOfNames */ ,
    0 /* IDispatch_Invoke_Proxy */ ,
    (void *) (INT_PTR) -1 /* IApp::get_currentVersion */ ,
    (void *) (INT_PTR) -1 /* IApp::get_nextVersion */ ,
    (void *) (INT_PTR) -1 /* IApp::get_appId */ ,
    (void *) (INT_PTR) -1 /* IApp::get_displayName */ ,
    (void *) (INT_PTR) -1 /* IApp::put_displayName */ ,
    (void *) (INT_PTR) -1 /* IApp::get_language */ ,
    (void *) (INT_PTR) -1 /* IApp::put_language */ ,
    (void *) (INT_PTR) -1 /* IApp::get_ap */ ,
    (void *) (INT_PTR) -1 /* IApp::put_ap */ ,
    (void *) (INT_PTR) -1 /* IApp::get_ttToken */ ,
    (void *) (INT_PTR) -1 /* IApp::put_ttToken */ ,
    (void *) (INT_PTR) -1 /* IApp::get_iid */ ,
    (void *) (INT_PTR) -1 /* IApp::put_iid */ ,
    (void *) (INT_PTR) -1 /* IApp::get_brandCode */ ,
    (void *) (INT_PTR) -1 /* IApp::put_brandCode */ ,
    (void *) (INT_PTR) -1 /* IApp::get_clientId */ ,
    (void *) (INT_PTR) -1 /* IApp::put_clientId */ ,
    (void *) (INT_PTR) -1 /* IApp::get_labels */ ,
    (void *) (INT_PTR) -1 /* IApp::put_labels */ ,
    (void *) (INT_PTR) -1 /* IApp::get_referralId */ ,
    (void *) (INT_PTR) -1 /* IApp::put_referralId */ ,
    (void *) (INT_PTR) -1 /* IApp::get_command */ ,
    (void *) (INT_PTR) -1 /* IApp::get_browserType */ ,
    (void *) (INT_PTR) -1 /* IApp::put_browserType */ ,
    (void *) (INT_PTR) -1 /* IApp::get_clientInstallData */ ,
    (void *) (INT_PTR) -1 /* IApp::put_clientInstallData */ ,
    (void *) (INT_PTR) -1 /* IApp::get_serverInstallDataIndex */ ,
    (void *) (INT_PTR) -1 /* IApp::put_serverInstallDataIndex */ ,
    (void *) (INT_PTR) -1 /* IApp::get_isEulaAccepted */ ,
    (void *) (INT_PTR) -1 /* IApp::put_isEulaAccepted */ ,
    (void *) (INT_PTR) -1 /* IApp::get_usageStatsEnable */ ,
    (void *) (INT_PTR) -1 /* IApp::put_usageStatsEnable */ ,
    (void *) (INT_PTR) -1 /* IApp::get_installTimeDiffSec */ ,
    (void *) (INT_PTR) -1 /* IApp::get_currentState */
};


static const PRPC_STUB_FUNCTION IApp_table[] =
{
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2
};

CInterfaceStubVtbl _IAppStubVtbl =
{
    &IID_IApp,
    &IApp_ServerInfo,
    41,
    &IApp_table[-3],
    CStdStubBuffer_DELEGATING_METHODS
};


/* Object interface: IApp2, ver. 0.0,
   GUID={0xA147722A,0x5568,0x4B84,{0xB4,0x01,0x86,0xD7,0x44,0x47,0x0C,0xBF}} */

#pragma code_seg(".orpc")
static const unsigned short IApp2_FormatStringOffsetTable[] =
    {
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    1394,
    1432,
    196,
    1470,
    1508,
    1546,
    1584,
    1622,
    1660,
    1698,
    1736,
    1774,
    1812,
    1850,
    1888,
    1926,
    1964,
    2002,
    2040,
    2078,
    2116,
    930,
    2154,
    2192,
    2230,
    2268,
    2306,
    2344,
    2382,
    2420,
    2458,
    2496,
    2534,
    2572,
    2610,
    2648
    };

static const MIDL_STUBLESS_PROXY_INFO IApp2_ProxyInfo =
    {
    &Object_StubDesc,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IApp2_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IApp2_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IApp2_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(43) _IApp2ProxyVtbl = 
{
    &IApp2_ProxyInfo,
    &IID_IApp2,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    0 /* IDispatch::GetTypeInfoCount */ ,
    0 /* IDispatch::GetTypeInfo */ ,
    0 /* IDispatch::GetIDsOfNames */ ,
    0 /* IDispatch_Invoke_Proxy */ ,
    (void *) (INT_PTR) -1 /* IApp::get_currentVersion */ ,
    (void *) (INT_PTR) -1 /* IApp::get_nextVersion */ ,
    (void *) (INT_PTR) -1 /* IApp::get_appId */ ,
    (void *) (INT_PTR) -1 /* IApp::get_displayName */ ,
    (void *) (INT_PTR) -1 /* IApp::put_displayName */ ,
    (void *) (INT_PTR) -1 /* IApp::get_language */ ,
    (void *) (INT_PTR) -1 /* IApp::put_language */ ,
    (void *) (INT_PTR) -1 /* IApp::get_ap */ ,
    (void *) (INT_PTR) -1 /* IApp::put_ap */ ,
    (void *) (INT_PTR) -1 /* IApp::get_ttToken */ ,
    (void *) (INT_PTR) -1 /* IApp::put_ttToken */ ,
    (void *) (INT_PTR) -1 /* IApp::get_iid */ ,
    (void *) (INT_PTR) -1 /* IApp::put_iid */ ,
    (void *) (INT_PTR) -1 /* IApp::get_brandCode */ ,
    (void *) (INT_PTR) -1 /* IApp::put_brandCode */ ,
    (void *) (INT_PTR) -1 /* IApp::get_clientId */ ,
    (void *) (INT_PTR) -1 /* IApp::put_clientId */ ,
    (void *) (INT_PTR) -1 /* IApp::get_labels */ ,
    (void *) (INT_PTR) -1 /* IApp::put_labels */ ,
    (void *) (INT_PTR) -1 /* IApp::get_referralId */ ,
    (void *) (INT_PTR) -1 /* IApp::put_referralId */ ,
    (void *) (INT_PTR) -1 /* IApp::get_command */ ,
    (void *) (INT_PTR) -1 /* IApp::get_browserType */ ,
    (void *) (INT_PTR) -1 /* IApp::put_browserType */ ,
    (void *) (INT_PTR) -1 /* IApp::get_clientInstallData */ ,
    (void *) (INT_PTR) -1 /* IApp::put_clientInstallData */ ,
    (void *) (INT_PTR) -1 /* IApp::get_serverInstallDataIndex */ ,
    (void *) (INT_PTR) -1 /* IApp::put_serverInstallDataIndex */ ,
    (void *) (INT_PTR) -1 /* IApp::get_isEulaAccepted */ ,
    (void *) (INT_PTR) -1 /* IApp::put_isEulaAccepted */ ,
    (void *) (INT_PTR) -1 /* IApp::get_usageStatsEnable */ ,
    (void *) (INT_PTR) -1 /* IApp::put_usageStatsEnable */ ,
    (void *) (INT_PTR) -1 /* IApp::get_installTimeDiffSec */ ,
    (void *) (INT_PTR) -1 /* IApp::get_currentState */ ,
    (void *) (INT_PTR) -1 /* IApp2::get_untrustedData */ ,
    (void *) (INT_PTR) -1 /* IApp2::put_untrustedData */
};


static const PRPC_STUB_FUNCTION IApp2_table[] =
{
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2
};

CInterfaceStubVtbl _IApp2StubVtbl =
{
    &IID_IApp2,
    &IApp2_ServerInfo,
    43,
    &IApp2_table[-3],
    CStdStubBuffer_DELEGATING_METHODS
};


/* Object interface: IAppCommand, ver. 0.0,
   GUID={0x4842EC21,0x0860,0x45B5,{0x99,0xF0,0xA1,0xE6,0xE7,0xC1,0x15,0x61}} */

#pragma code_seg(".orpc")
static const unsigned short IAppCommand_FormatStringOffsetTable[] =
    {
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    2686,
    2724,
    2762,
    2800
    };

static const MIDL_STUBLESS_PROXY_INFO IAppCommand_ProxyInfo =
    {
    &Object_StubDesc,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IAppCommand_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IAppCommand_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IAppCommand_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(11) _IAppCommandProxyVtbl = 
{
    &IAppCommand_ProxyInfo,
    &IID_IAppCommand,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    0 /* IDispatch::GetTypeInfoCount */ ,
    0 /* IDispatch::GetTypeInfo */ ,
    0 /* IDispatch::GetIDsOfNames */ ,
    0 /* IDispatch_Invoke_Proxy */ ,
    (void *) (INT_PTR) -1 /* IAppCommand::get_isWebAccessible */ ,
    (void *) (INT_PTR) -1 /* IAppCommand::get_status */ ,
    (void *) (INT_PTR) -1 /* IAppCommand::get_exitCode */ ,
    (void *) (INT_PTR) -1 /* IAppCommand::execute */
};


static const PRPC_STUB_FUNCTION IAppCommand_table[] =
{
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2
};

CInterfaceStubVtbl _IAppCommandStubVtbl =
{
    &IID_IAppCommand,
    &IAppCommand_ServerInfo,
    11,
    &IAppCommand_table[-3],
    CStdStubBuffer_DELEGATING_METHODS
};


/* Object interface: IAppCommand2, ver. 0.0,
   GUID={0xC11C073F,0xE6D0,0x4EF7,{0x89,0x7B,0xAA,0xF5,0x24,0x98,0xCD,0x2F}} */

#pragma code_seg(".orpc")
static const unsigned short IAppCommand2_FormatStringOffsetTable[] =
    {
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    2686,
    2724,
    2762,
    2800,
    272
    };

static const MIDL_STUBLESS_PROXY_INFO IAppCommand2_ProxyInfo =
    {
    &Object_StubDesc,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IAppCommand2_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IAppCommand2_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IAppCommand2_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(12) _IAppCommand2ProxyVtbl = 
{
    &IAppCommand2_ProxyInfo,
    &IID_IAppCommand2,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    0 /* IDispatch::GetTypeInfoCount */ ,
    0 /* IDispatch::GetTypeInfo */ ,
    0 /* IDispatch::GetIDsOfNames */ ,
    0 /* IDispatch_Invoke_Proxy */ ,
    (void *) (INT_PTR) -1 /* IAppCommand::get_isWebAccessible */ ,
    (void *) (INT_PTR) -1 /* IAppCommand::get_status */ ,
    (void *) (INT_PTR) -1 /* IAppCommand::get_exitCode */ ,
    (void *) (INT_PTR) -1 /* IAppCommand::execute */ ,
    (void *) (INT_PTR) -1 /* IAppCommand2::get_output */
};


static const PRPC_STUB_FUNCTION IAppCommand2_table[] =
{
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2
};

CInterfaceStubVtbl _IAppCommand2StubVtbl =
{
    &IID_IAppCommand2,
    &IAppCommand2_ServerInfo,
    12,
    &IAppCommand2_table[-3],
    CStdStubBuffer_DELEGATING_METHODS
};


/* Object interface: IAppVersion, ver. 0.0,
   GUID={0xF234546B,0xDACD,0x4374,{0x97,0xCF,0x7B,0xAD,0xFA,0xB7,0x67,0x66}} */

#pragma code_seg(".orpc")
static const unsigned short IAppVersion_FormatStringOffsetTable[] =
    {
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    120,
    2724,
    2886
    };

static const MIDL_STUBLESS_PROXY_INFO IAppVersion_ProxyInfo =
    {
    &Object_StubDesc,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IAppVersion_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IAppVersion_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IAppVersion_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(10) _IAppVersionProxyVtbl = 
{
    &IAppVersion_ProxyInfo,
    &IID_IAppVersion,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    0 /* IDispatch::GetTypeInfoCount */ ,
    0 /* IDispatch::GetTypeInfo */ ,
    0 /* IDispatch::GetIDsOfNames */ ,
    0 /* IDispatch_Invoke_Proxy */ ,
    (void *) (INT_PTR) -1 /* IAppVersion::get_version */ ,
    (void *) (INT_PTR) -1 /* IAppVersion::get_packageCount */ ,
    (void *) (INT_PTR) -1 /* IAppVersion::get_package */
};


static const PRPC_STUB_FUNCTION IAppVersion_table[] =
{
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2
};

CInterfaceStubVtbl _IAppVersionStubVtbl =
{
    &IID_IAppVersion,
    &IAppVersion_ServerInfo,
    10,
    &IAppVersion_table[-3],
    CStdStubBuffer_DELEGATING_METHODS
};


/* Object interface: IPackage, ver. 0.0,
   GUID={0xC663DEBB,0xF082,0x4971,{0x9F,0x6E,0x35,0xDE,0x45,0xC9,0x6F,0x4E}} */

#pragma code_seg(".orpc")
static const unsigned short IPackage_FormatStringOffsetTable[] =
    {
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    2930,
    2968,
    196
    };

static const MIDL_STUBLESS_PROXY_INFO IPackage_ProxyInfo =
    {
    &Object_StubDesc,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IPackage_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IPackage_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IPackage_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(10) _IPackageProxyVtbl = 
{
    &IPackage_ProxyInfo,
    &IID_IPackage,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    0 /* IDispatch::GetTypeInfoCount */ ,
    0 /* IDispatch::GetTypeInfo */ ,
    0 /* IDispatch::GetIDsOfNames */ ,
    0 /* IDispatch_Invoke_Proxy */ ,
    (void *) (INT_PTR) -1 /* IPackage::get */ ,
    (void *) (INT_PTR) -1 /* IPackage::get_isAvailable */ ,
    (void *) (INT_PTR) -1 /* IPackage::get_filename */
};


static const PRPC_STUB_FUNCTION IPackage_table[] =
{
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2
};

CInterfaceStubVtbl _IPackageStubVtbl =
{
    &IID_IPackage,
    &IPackage_ServerInfo,
    10,
    &IPackage_table[-3],
    CStdStubBuffer_DELEGATING_METHODS
};


/* Object interface: ICurrentState, ver. 0.0,
   GUID={0xE6836CFF,0x5949,0x44BC,{0xB6,0xBE,0x9C,0x8C,0x48,0xDD,0x8D,0x97}} */

#pragma code_seg(".orpc")
static const unsigned short ICurrentState_FormatStringOffsetTable[] =
    {
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    0,
    3006,
    2762,
    3044,
    3082,
    3120,
    3158,
    3196,
    3234,
    3272,
    3310,
    1774,
    3348,
    3386,
    3424,
    1926,
    728
    };

static const MIDL_STUBLESS_PROXY_INFO ICurrentState_ProxyInfo =
    {
    &Object_StubDesc,
    google_update_idl__MIDL_ProcFormatString.Format,
    &ICurrentState_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICurrentState_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    google_update_idl__MIDL_ProcFormatString.Format,
    &ICurrentState_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(24) _ICurrentStateProxyVtbl = 
{
    &ICurrentState_ProxyInfo,
    &IID_ICurrentState,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    0 /* IDispatch::GetTypeInfoCount */ ,
    0 /* IDispatch::GetTypeInfo */ ,
    0 /* IDispatch::GetIDsOfNames */ ,
    0 /* IDispatch_Invoke_Proxy */ ,
    (void *) (INT_PTR) -1 /* ICurrentState::get_stateValue */ ,
    (void *) (INT_PTR) -1 /* ICurrentState::get_availableVersion */ ,
    (void *) (INT_PTR) -1 /* ICurrentState::get_bytesDownloaded */ ,
    (void *) (INT_PTR) -1 /* ICurrentState::get_totalBytesToDownload */ ,
    (void *) (INT_PTR) -1 /* ICurrentState::get_downloadTimeRemainingMs */ ,
    (void *) (INT_PTR) -1 /* ICurrentState::get_nextRetryTime */ ,
    (void *) (INT_PTR) -1 /* ICurrentState::get_installProgress */ ,
    (void *) (INT_PTR) -1 /* ICurrentState::get_installTimeRemainingMs */ ,
    (void *) (INT_PTR) -1 /* ICurrentState::get_isCanceled */ ,
    (void *) (INT_PTR) -1 /* ICurrentState::get_errorCode */ ,
    (void *) (INT_PTR) -1 /* ICurrentState::get_extraCode1 */ ,
    (void *) (INT_PTR) -1 /* ICurrentState::get_completionMessage */ ,
    (void *) (INT_PTR) -1 /* ICurrentState::get_installerResultCode */ ,
    (void *) (INT_PTR) -1 /* ICurrentState::get_installerResultExtraCode1 */ ,
    (void *) (INT_PTR) -1 /* ICurrentState::get_postInstallLaunchCommandLine */ ,
    (void *) (INT_PTR) -1 /* ICurrentState::get_postInstallUrl */ ,
    (void *) (INT_PTR) -1 /* ICurrentState::get_postInstallAction */
};


static const PRPC_STUB_FUNCTION ICurrentState_table[] =
{
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2
};

CInterfaceStubVtbl _ICurrentStateStubVtbl =
{
    &IID_ICurrentState,
    &ICurrentState_ServerInfo,
    24,
    &ICurrentState_table[-3],
    CStdStubBuffer_DELEGATING_METHODS
};


/* Object interface: IRegistrationUpdateHook, ver. 0.0,
   GUID={0x48B5E6B2,0x9383,0x4B1E,{0xAA,0xE7,0x72,0x0C,0x47,0x79,0xAB,0xA6}} */

#pragma code_seg(".orpc")
static const unsigned short IRegistrationUpdateHook_FormatStringOffsetTable[] =
    {
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    3462
    };

static const MIDL_STUBLESS_PROXY_INFO IRegistrationUpdateHook_ProxyInfo =
    {
    &Object_StubDesc,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IRegistrationUpdateHook_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IRegistrationUpdateHook_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IRegistrationUpdateHook_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(8) _IRegistrationUpdateHookProxyVtbl = 
{
    &IRegistrationUpdateHook_ProxyInfo,
    &IID_IRegistrationUpdateHook,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    0 /* IDispatch::GetTypeInfoCount */ ,
    0 /* IDispatch::GetTypeInfo */ ,
    0 /* IDispatch::GetIDsOfNames */ ,
    0 /* IDispatch_Invoke_Proxy */ ,
    (void *) (INT_PTR) -1 /* IRegistrationUpdateHook::UpdateRegistry */
};


static const PRPC_STUB_FUNCTION IRegistrationUpdateHook_table[] =
{
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    NdrStubCall2
};

CInterfaceStubVtbl _IRegistrationUpdateHookStubVtbl =
{
    &IID_IRegistrationUpdateHook,
    &IRegistrationUpdateHook_ServerInfo,
    8,
    &IRegistrationUpdateHook_table[-3],
    CStdStubBuffer_DELEGATING_METHODS
};


/* Object interface: ICredentialDialog, ver. 0.0,
   GUID={0x660130E8,0x74E4,0x4821,{0xA6,0xFD,0x4E,0x9A,0x86,0xE0,0x64,0x70}} */

#pragma code_seg(".orpc")
static const unsigned short ICredentialDialog_FormatStringOffsetTable[] =
    {
    3506
    };

static const MIDL_STUBLESS_PROXY_INFO ICredentialDialog_ProxyInfo =
    {
    &Object_StubDesc,
    google_update_idl__MIDL_ProcFormatString.Format,
    &ICredentialDialog_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICredentialDialog_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    google_update_idl__MIDL_ProcFormatString.Format,
    &ICredentialDialog_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(4) _ICredentialDialogProxyVtbl = 
{
    &ICredentialDialog_ProxyInfo,
    &IID_ICredentialDialog,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICredentialDialog::QueryUserForCredentials */
};

const CInterfaceStubVtbl _ICredentialDialogStubVtbl =
{
    &IID_ICredentialDialog,
    &ICredentialDialog_ServerInfo,
    4,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: IGoogleUpdate3Web, ver. 0.0,
   GUID={0xC9190589,0xECEC,0x43F8,{0x8A,0xEC,0x62,0x49,0x6B,0xB8,0x7B,0x26}} */

#pragma code_seg(".orpc")
static const unsigned short IGoogleUpdate3Web_FormatStringOffsetTable[] =
    {
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    1394
    };

static const MIDL_STUBLESS_PROXY_INFO IGoogleUpdate3Web_ProxyInfo =
    {
    &Object_StubDesc,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IGoogleUpdate3Web_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IGoogleUpdate3Web_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IGoogleUpdate3Web_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(8) _IGoogleUpdate3WebProxyVtbl = 
{
    &IGoogleUpdate3Web_ProxyInfo,
    &IID_IGoogleUpdate3Web,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    0 /* IDispatch::GetTypeInfoCount */ ,
    0 /* IDispatch::GetTypeInfo */ ,
    0 /* IDispatch::GetIDsOfNames */ ,
    0 /* IDispatch_Invoke_Proxy */ ,
    (void *) (INT_PTR) -1 /* IGoogleUpdate3Web::createAppBundleWeb */
};


static const PRPC_STUB_FUNCTION IGoogleUpdate3Web_table[] =
{
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    NdrStubCall2
};

CInterfaceStubVtbl _IGoogleUpdate3WebStubVtbl =
{
    &IID_IGoogleUpdate3Web,
    &IGoogleUpdate3Web_ServerInfo,
    8,
    &IGoogleUpdate3Web_table[-3],
    CStdStubBuffer_DELEGATING_METHODS
};


/* Object interface: IGoogleUpdate3WebSecurity, ver. 0.0,
   GUID={0x24D704AD,0xAC42,0x49F2,{0xBB,0x4F,0x68,0xBA,0x77,0xC9,0x8E,0x91}} */

#pragma code_seg(".orpc")
static const unsigned short IGoogleUpdate3WebSecurity_FormatStringOffsetTable[] =
    {
    3568
    };

static const MIDL_STUBLESS_PROXY_INFO IGoogleUpdate3WebSecurity_ProxyInfo =
    {
    &Object_StubDesc,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IGoogleUpdate3WebSecurity_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IGoogleUpdate3WebSecurity_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IGoogleUpdate3WebSecurity_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(4) _IGoogleUpdate3WebSecurityProxyVtbl = 
{
    &IGoogleUpdate3WebSecurity_ProxyInfo,
    &IID_IGoogleUpdate3WebSecurity,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* IGoogleUpdate3WebSecurity::setOriginURL */
};

const CInterfaceStubVtbl _IGoogleUpdate3WebSecurityStubVtbl =
{
    &IID_IGoogleUpdate3WebSecurity,
    &IGoogleUpdate3WebSecurity_ServerInfo,
    4,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: IAppBundleWeb, ver. 0.0,
   GUID={0x852A0F87,0xD117,0x4B7C,{0xAB,0xA9,0x2F,0x76,0xD9,0x1B,0xCB,0x9D}} */

#pragma code_seg(".orpc")
static const unsigned short IAppBundleWeb_FormatStringOffsetTable[] =
    {
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    3606,
    158,
    3662,
    1470,
    1508,
    3694,
    3158,
    3732,
    3776,
    3808,
    3840,
    3872,
    3904,
    3936,
    3968,
    4000,
    4044
    };

static const MIDL_STUBLESS_PROXY_INFO IAppBundleWeb_ProxyInfo =
    {
    &Object_StubDesc,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IAppBundleWeb_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IAppBundleWeb_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IAppBundleWeb_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(24) _IAppBundleWebProxyVtbl = 
{
    &IAppBundleWeb_ProxyInfo,
    &IID_IAppBundleWeb,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    0 /* IDispatch::GetTypeInfoCount */ ,
    0 /* IDispatch::GetTypeInfo */ ,
    0 /* IDispatch::GetIDsOfNames */ ,
    0 /* IDispatch_Invoke_Proxy */ ,
    (void *) (INT_PTR) -1 /* IAppBundleWeb::createApp */ ,
    (void *) (INT_PTR) -1 /* IAppBundleWeb::createInstalledApp */ ,
    (void *) (INT_PTR) -1 /* IAppBundleWeb::createAllInstalledApps */ ,
    (void *) (INT_PTR) -1 /* IAppBundleWeb::get_displayLanguage */ ,
    (void *) (INT_PTR) -1 /* IAppBundleWeb::put_displayLanguage */ ,
    (void *) (INT_PTR) -1 /* IAppBundleWeb::put_parentHWND */ ,
    (void *) (INT_PTR) -1 /* IAppBundleWeb::get_length */ ,
    (void *) (INT_PTR) -1 /* IAppBundleWeb::get_appWeb */ ,
    (void *) (INT_PTR) -1 /* IAppBundleWeb::initialize */ ,
    (void *) (INT_PTR) -1 /* IAppBundleWeb::checkForUpdate */ ,
    (void *) (INT_PTR) -1 /* IAppBundleWeb::download */ ,
    (void *) (INT_PTR) -1 /* IAppBundleWeb::install */ ,
    (void *) (INT_PTR) -1 /* IAppBundleWeb::pause */ ,
    (void *) (INT_PTR) -1 /* IAppBundleWeb::resume */ ,
    (void *) (INT_PTR) -1 /* IAppBundleWeb::cancel */ ,
    (void *) (INT_PTR) -1 /* IAppBundleWeb::downloadPackage */ ,
    (void *) (INT_PTR) -1 /* IAppBundleWeb::get_currentState */
};


static const PRPC_STUB_FUNCTION IAppBundleWeb_table[] =
{
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2
};

CInterfaceStubVtbl _IAppBundleWebStubVtbl =
{
    &IID_IAppBundleWeb,
    &IAppBundleWeb_ServerInfo,
    24,
    &IAppBundleWeb_table[-3],
    CStdStubBuffer_DELEGATING_METHODS
};


/* Object interface: IAppWeb, ver. 0.0,
   GUID={0xFB43AAD0,0xDDBA,0x4D01,{0xA3,0xE0,0xFA,0xB1,0x00,0xE7,0x92,0x6B}} */

#pragma code_seg(".orpc")
static const unsigned short IAppWeb_FormatStringOffsetTable[] =
    {
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    120,
    1432,
    82,
    4082,
    4126,
    4158,
    4196,
    4228,
    424,
    462
    };

static const MIDL_STUBLESS_PROXY_INFO IAppWeb_ProxyInfo =
    {
    &Object_StubDesc,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IAppWeb_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IAppWeb_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IAppWeb_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(17) _IAppWebProxyVtbl = 
{
    &IAppWeb_ProxyInfo,
    &IID_IAppWeb,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    0 /* IDispatch::GetTypeInfoCount */ ,
    0 /* IDispatch::GetTypeInfo */ ,
    0 /* IDispatch::GetIDsOfNames */ ,
    0 /* IDispatch_Invoke_Proxy */ ,
    (void *) (INT_PTR) -1 /* IAppWeb::get_appId */ ,
    (void *) (INT_PTR) -1 /* IAppWeb::get_currentVersionWeb */ ,
    (void *) (INT_PTR) -1 /* IAppWeb::get_nextVersionWeb */ ,
    (void *) (INT_PTR) -1 /* IAppWeb::get_command */ ,
    (void *) (INT_PTR) -1 /* IAppWeb::cancel */ ,
    (void *) (INT_PTR) -1 /* IAppWeb::get_currentState */ ,
    (void *) (INT_PTR) -1 /* IAppWeb::launch */ ,
    (void *) (INT_PTR) -1 /* IAppWeb::uninstall */ ,
    (void *) (INT_PTR) -1 /* IAppWeb::get_serverInstallDataIndex */ ,
    (void *) (INT_PTR) -1 /* IAppWeb::put_serverInstallDataIndex */
};


static const PRPC_STUB_FUNCTION IAppWeb_table[] =
{
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2
};

CInterfaceStubVtbl _IAppWebStubVtbl =
{
    &IID_IAppWeb,
    &IAppWeb_ServerInfo,
    17,
    &IAppWeb_table[-3],
    CStdStubBuffer_DELEGATING_METHODS
};


/* Object interface: IAppCommandWeb, ver. 0.0,
   GUID={0x19F4616B,0xB7DD,0x4B3F,{0x80,0x84,0xC8,0x1C,0x5C,0x77,0xAA,0xA4}} */

#pragma code_seg(".orpc")
static const unsigned short IAppCommandWeb_FormatStringOffsetTable[] =
    {
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    0,
    2724,
    196,
    2800
    };

static const MIDL_STUBLESS_PROXY_INFO IAppCommandWeb_ProxyInfo =
    {
    &Object_StubDesc,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IAppCommandWeb_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IAppCommandWeb_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IAppCommandWeb_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(11) _IAppCommandWebProxyVtbl = 
{
    &IAppCommandWeb_ProxyInfo,
    &IID_IAppCommandWeb,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    0 /* IDispatch::GetTypeInfoCount */ ,
    0 /* IDispatch::GetTypeInfo */ ,
    0 /* IDispatch::GetIDsOfNames */ ,
    0 /* IDispatch_Invoke_Proxy */ ,
    (void *) (INT_PTR) -1 /* IAppCommandWeb::get_status */ ,
    (void *) (INT_PTR) -1 /* IAppCommandWeb::get_exitCode */ ,
    (void *) (INT_PTR) -1 /* IAppCommandWeb::get_output */ ,
    (void *) (INT_PTR) -1 /* IAppCommandWeb::execute */
};


static const PRPC_STUB_FUNCTION IAppCommandWeb_table[] =
{
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2
};

CInterfaceStubVtbl _IAppCommandWebStubVtbl =
{
    &IID_IAppCommandWeb,
    &IAppCommandWeb_ServerInfo,
    11,
    &IAppCommandWeb_table[-3],
    CStdStubBuffer_DELEGATING_METHODS
};


/* Object interface: IAppVersionWeb, ver. 0.0,
   GUID={0x35A4470F,0x5EEC,0x4715,{0xA2,0xDC,0x6A,0xA9,0xF8,0xE2,0x11,0x83}} */

#pragma code_seg(".orpc")
static const unsigned short IAppVersionWeb_FormatStringOffsetTable[] =
    {
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    120,
    2724,
    2886
    };

static const MIDL_STUBLESS_PROXY_INFO IAppVersionWeb_ProxyInfo =
    {
    &Object_StubDesc,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IAppVersionWeb_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IAppVersionWeb_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IAppVersionWeb_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(10) _IAppVersionWebProxyVtbl = 
{
    &IAppVersionWeb_ProxyInfo,
    &IID_IAppVersionWeb,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    0 /* IDispatch::GetTypeInfoCount */ ,
    0 /* IDispatch::GetTypeInfo */ ,
    0 /* IDispatch::GetIDsOfNames */ ,
    0 /* IDispatch_Invoke_Proxy */ ,
    (void *) (INT_PTR) -1 /* IAppVersionWeb::get_version */ ,
    (void *) (INT_PTR) -1 /* IAppVersionWeb::get_packageCount */ ,
    (void *) (INT_PTR) -1 /* IAppVersionWeb::get_packageWeb */
};


static const PRPC_STUB_FUNCTION IAppVersionWeb_table[] =
{
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2
};

CInterfaceStubVtbl _IAppVersionWebStubVtbl =
{
    &IID_IAppVersionWeb,
    &IAppVersionWeb_ServerInfo,
    10,
    &IAppVersionWeb_table[-3],
    CStdStubBuffer_DELEGATING_METHODS
};


/* Object interface: ICoCreateAsyncStatus, ver. 0.0,
   GUID={0x7A24060E,0x533F,0x4962,{0x9E,0x15,0x34,0xBD,0x82,0x55,0x5F,0xA7}} */

#pragma code_seg(".orpc")
static const unsigned short ICoCreateAsyncStatus_FormatStringOffsetTable[] =
    {
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    2686,
    2724,
    82
    };

static const MIDL_STUBLESS_PROXY_INFO ICoCreateAsyncStatus_ProxyInfo =
    {
    &Object_StubDesc,
    google_update_idl__MIDL_ProcFormatString.Format,
    &ICoCreateAsyncStatus_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICoCreateAsyncStatus_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    google_update_idl__MIDL_ProcFormatString.Format,
    &ICoCreateAsyncStatus_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(10) _ICoCreateAsyncStatusProxyVtbl = 
{
    &ICoCreateAsyncStatus_ProxyInfo,
    &IID_ICoCreateAsyncStatus,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    0 /* IDispatch::GetTypeInfoCount */ ,
    0 /* IDispatch::GetTypeInfo */ ,
    0 /* IDispatch::GetIDsOfNames */ ,
    0 /* IDispatch_Invoke_Proxy */ ,
    (void *) (INT_PTR) -1 /* ICoCreateAsyncStatus::get_isDone */ ,
    (void *) (INT_PTR) -1 /* ICoCreateAsyncStatus::get_completionHResult */ ,
    (void *) (INT_PTR) -1 /* ICoCreateAsyncStatus::get_createdInstance */
};


static const PRPC_STUB_FUNCTION ICoCreateAsyncStatus_table[] =
{
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    STUB_FORWARDING_FUNCTION,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2
};

CInterfaceStubVtbl _ICoCreateAsyncStatusStubVtbl =
{
    &IID_ICoCreateAsyncStatus,
    &ICoCreateAsyncStatus_ServerInfo,
    10,
    &ICoCreateAsyncStatus_table[-3],
    CStdStubBuffer_DELEGATING_METHODS
};


/* Object interface: ICoCreateAsync, ver. 0.0,
   GUID={0xDD84E356,0x3D21,0x44C8,{0x83,0xDD,0x6B,0xEE,0xC2,0x2F,0xA4,0x27}} */

#pragma code_seg(".orpc")
static const unsigned short ICoCreateAsync_FormatStringOffsetTable[] =
    {
    4260
    };

static const MIDL_STUBLESS_PROXY_INFO ICoCreateAsync_ProxyInfo =
    {
    &Object_StubDesc,
    google_update_idl__MIDL_ProcFormatString.Format,
    &ICoCreateAsync_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO ICoCreateAsync_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    google_update_idl__MIDL_ProcFormatString.Format,
    &ICoCreateAsync_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(4) _ICoCreateAsyncProxyVtbl = 
{
    &ICoCreateAsync_ProxyInfo,
    &IID_ICoCreateAsync,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* ICoCreateAsync::createOmahaMachineServerAsync */
};

const CInterfaceStubVtbl _ICoCreateAsyncStubVtbl =
{
    &IID_ICoCreateAsync,
    &ICoCreateAsync_ServerInfo,
    4,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: IBrowserHttpRequest2, ver. 0.0,
   GUID={0xAAE4AD28,0x500D,0x43BA,{0x9F,0x54,0x73,0x0C,0xA1,0x46,0xC1,0x90}} */

#pragma code_seg(".orpc")
static const unsigned short IBrowserHttpRequest2_FormatStringOffsetTable[] =
    {
    4310
    };

static const MIDL_STUBLESS_PROXY_INFO IBrowserHttpRequest2_ProxyInfo =
    {
    &Object_StubDesc,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IBrowserHttpRequest2_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IBrowserHttpRequest2_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IBrowserHttpRequest2_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(4) _IBrowserHttpRequest2ProxyVtbl = 
{
    &IBrowserHttpRequest2_ProxyInfo,
    &IID_IBrowserHttpRequest2,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* IBrowserHttpRequest2::Send */
};

const CInterfaceStubVtbl _IBrowserHttpRequest2StubVtbl =
{
    &IID_IBrowserHttpRequest2,
    &IBrowserHttpRequest2_ServerInfo,
    4,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: IProcessLauncher, ver. 0.0,
   GUID={0x70E5ECF5,0x2CA7,0x4019,{0x9B,0x23,0x91,0x67,0x89,0xA1,0x3C,0x2C}} */

#pragma code_seg(".orpc")
static const unsigned short IProcessLauncher_FormatStringOffsetTable[] =
    {
    4384,
    4422,
    4466
    };

static const MIDL_STUBLESS_PROXY_INFO IProcessLauncher_ProxyInfo =
    {
    &Object_StubDesc,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IProcessLauncher_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IProcessLauncher_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IProcessLauncher_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(6) _IProcessLauncherProxyVtbl = 
{
    &IProcessLauncher_ProxyInfo,
    &IID_IProcessLauncher,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* IProcessLauncher::LaunchCmdLine */ ,
    (void *) (INT_PTR) -1 /* IProcessLauncher::LaunchBrowser */ ,
    (void *) (INT_PTR) -1 /* IProcessLauncher::LaunchCmdElevated */
};

const CInterfaceStubVtbl _IProcessLauncherStubVtbl =
{
    &IID_IProcessLauncher,
    &IProcessLauncher_ServerInfo,
    6,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: IProcessLauncher2, ver. 0.0,
   GUID={0xD5627FC9,0xE2F0,0x484B,{0x89,0xA4,0x5D,0xAC,0xFE,0x7F,0xAA,0xD3}} */

#pragma code_seg(".orpc")
static const unsigned short IProcessLauncher2_FormatStringOffsetTable[] =
    {
    4384,
    4422,
    4466,
    4522
    };

static const MIDL_STUBLESS_PROXY_INFO IProcessLauncher2_ProxyInfo =
    {
    &Object_StubDesc,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IProcessLauncher2_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IProcessLauncher2_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IProcessLauncher2_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(7) _IProcessLauncher2ProxyVtbl = 
{
    &IProcessLauncher2_ProxyInfo,
    &IID_IProcessLauncher2,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* IProcessLauncher::LaunchCmdLine */ ,
    (void *) (INT_PTR) -1 /* IProcessLauncher::LaunchBrowser */ ,
    (void *) (INT_PTR) -1 /* IProcessLauncher::LaunchCmdElevated */ ,
    (void *) (INT_PTR) -1 /* IProcessLauncher2::LaunchCmdLineEx */
};

const CInterfaceStubVtbl _IProcessLauncher2StubVtbl =
{
    &IID_IProcessLauncher2,
    &IProcessLauncher2_ServerInfo,
    7,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: IOneClickProcessLauncher, ver. 0.0,
   GUID={0x0FF491FB,0xE162,0x4E0B,{0x9C,0xA3,0x14,0x64,0xB0,0xCF,0x9F,0x20}} */

#pragma code_seg(".orpc")
static const unsigned short IOneClickProcessLauncher_FormatStringOffsetTable[] =
    {
    4578
    };

static const MIDL_STUBLESS_PROXY_INFO IOneClickProcessLauncher_ProxyInfo =
    {
    &Object_StubDesc,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IOneClickProcessLauncher_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IOneClickProcessLauncher_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IOneClickProcessLauncher_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(4) _IOneClickProcessLauncherProxyVtbl = 
{
    &IOneClickProcessLauncher_ProxyInfo,
    &IID_IOneClickProcessLauncher,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* IOneClickProcessLauncher::LaunchAppCommand */
};

const CInterfaceStubVtbl _IOneClickProcessLauncherStubVtbl =
{
    &IID_IOneClickProcessLauncher,
    &IOneClickProcessLauncher_ServerInfo,
    4,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Standard interface: __MIDL_itf_google_update_idl_0000_0023, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: IProgressWndEvents, ver. 0.0,
   GUID={0x8504FB26,0xFC3E,0x4C1C,{0x9C,0x94,0x46,0xEC,0x93,0xE6,0xBA,0x63}} */

#pragma code_seg(".orpc")
static const unsigned short IProgressWndEvents_FormatStringOffsetTable[] =
    {
    4622,
    4654,
    4686,
    4718,
    4750,
    4782
    };

static const MIDL_STUBLESS_PROXY_INFO IProgressWndEvents_ProxyInfo =
    {
    &Object_StubDesc,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IProgressWndEvents_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IProgressWndEvents_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IProgressWndEvents_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(9) _IProgressWndEventsProxyVtbl = 
{
    &IProgressWndEvents_ProxyInfo,
    &IID_IProgressWndEvents,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* IProgressWndEvents::DoClose */ ,
    (void *) (INT_PTR) -1 /* IProgressWndEvents::DoPause */ ,
    (void *) (INT_PTR) -1 /* IProgressWndEvents::DoResume */ ,
    (void *) (INT_PTR) -1 /* IProgressWndEvents::DoRestartBrowsers */ ,
    (void *) (INT_PTR) -1 /* IProgressWndEvents::DoReboot */ ,
    (void *) (INT_PTR) -1 /* IProgressWndEvents::DoLaunchBrowser */
};

const CInterfaceStubVtbl _IProgressWndEventsStubVtbl =
{
    &IID_IProgressWndEvents,
    &IProgressWndEvents_ServerInfo,
    9,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: IJobObserver, ver. 0.0,
   GUID={0x7CFC4E00,0x1C9D,0x443D,{0xB5,0xBE,0xCE,0xEE,0xAC,0x14,0x43,0xAF}} */

#pragma code_seg(".orpc")
static const unsigned short IJobObserver_FormatStringOffsetTable[] =
    {
    4622,
    4654,
    4820,
    4718,
    4858,
    4902,
    3662,
    4934,
    4966,
    5010
    };

static const MIDL_STUBLESS_PROXY_INFO IJobObserver_ProxyInfo =
    {
    &Object_StubDesc,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IJobObserver_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IJobObserver_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IJobObserver_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(13) _IJobObserverProxyVtbl = 
{
    &IJobObserver_ProxyInfo,
    &IID_IJobObserver,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* IJobObserver::OnShow */ ,
    (void *) (INT_PTR) -1 /* IJobObserver::OnCheckingForUpdate */ ,
    (void *) (INT_PTR) -1 /* IJobObserver::OnUpdateAvailable */ ,
    (void *) (INT_PTR) -1 /* IJobObserver::OnWaitingToDownload */ ,
    (void *) (INT_PTR) -1 /* IJobObserver::OnDownloading */ ,
    (void *) (INT_PTR) -1 /* IJobObserver::OnWaitingToInstall */ ,
    (void *) (INT_PTR) -1 /* IJobObserver::OnInstalling */ ,
    (void *) (INT_PTR) -1 /* IJobObserver::OnPause */ ,
    (void *) (INT_PTR) -1 /* IJobObserver::OnComplete */ ,
    (void *) (INT_PTR) -1 /* IJobObserver::SetEventSink */
};

const CInterfaceStubVtbl _IJobObserverStubVtbl =
{
    &IID_IJobObserver,
    &IJobObserver_ServerInfo,
    13,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: IJobObserver2, ver. 0.0,
   GUID={0x91B050A9,0x5A49,0x4249,{0xA8,0xC8,0xB4,0x39,0x09,0x61,0xA9,0x12}} */

#pragma code_seg(".orpc")
static const unsigned short IJobObserver2_FormatStringOffsetTable[] =
    {
    5048
    };

static const MIDL_STUBLESS_PROXY_INFO IJobObserver2_ProxyInfo =
    {
    &Object_StubDesc,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IJobObserver2_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IJobObserver2_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IJobObserver2_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(4) _IJobObserver2ProxyVtbl = 
{
    &IJobObserver2_ProxyInfo,
    &IID_IJobObserver2,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* IJobObserver2::OnInstalling2 */
};

const CInterfaceStubVtbl _IJobObserver2StubVtbl =
{
    &IID_IJobObserver2,
    &IJobObserver2_ServerInfo,
    4,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: IGoogleUpdate, ver. 0.0,
   GUID={0x4C929BFE,0x4FA4,0x488D,{0xB1,0xE2,0x82,0xEC,0xD6,0xF0,0x76,0xC8}} */

#pragma code_seg(".orpc")
static const unsigned short IGoogleUpdate_FormatStringOffsetTable[] =
    {
    5092,
    5136
    };

static const MIDL_STUBLESS_PROXY_INFO IGoogleUpdate_ProxyInfo =
    {
    &Object_StubDesc,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IGoogleUpdate_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IGoogleUpdate_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IGoogleUpdate_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(5) _IGoogleUpdateProxyVtbl = 
{
    &IGoogleUpdate_ProxyInfo,
    &IID_IGoogleUpdate,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* IGoogleUpdate::CheckForUpdate */ ,
    (void *) (INT_PTR) -1 /* IGoogleUpdate::Update */
};

const CInterfaceStubVtbl _IGoogleUpdateStubVtbl =
{
    &IID_IGoogleUpdate,
    &IGoogleUpdate_ServerInfo,
    5,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: IGoogleUpdateCore, ver. 0.0,
   GUID={0x10479D64,0x2C5F,0x46CD,{0x9B,0xC8,0xFD,0x04,0xFF,0x4D,0x02,0xD8}} */

#pragma code_seg(".orpc")
static const unsigned short IGoogleUpdateCore_FormatStringOffsetTable[] =
    {
    5180
    };

static const MIDL_STUBLESS_PROXY_INFO IGoogleUpdateCore_ProxyInfo =
    {
    &Object_StubDesc,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IGoogleUpdateCore_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IGoogleUpdateCore_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IGoogleUpdateCore_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(4) _IGoogleUpdateCoreProxyVtbl = 
{
    &IGoogleUpdateCore_ProxyInfo,
    &IID_IGoogleUpdateCore,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* IGoogleUpdateCore::LaunchCmdElevated */
};

const CInterfaceStubVtbl _IGoogleUpdateCoreStubVtbl =
{
    &IID_IGoogleUpdateCore,
    &IGoogleUpdateCore_ServerInfo,
    4,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};

static const MIDL_STUB_DESC Object_StubDesc = 
    {
    0,
    NdrOleAllocate,
    NdrOleFree,
    0,
    0,
    0,
    0,
    0,
    google_update_idl__MIDL_TypeFormatString.Format,
    1, /* -error bounds_check flag */
    0x50002, /* Ndr library version */
    0,
    0x801026e, /* MIDL Version 8.1.622 */
    0,
    UserMarshalRoutines,
    0,  /* notify & notify_flag routine table */
    0x1, /* MIDL flag */
    0, /* cs routines */
    0,   /* proxy/server info */
    0
    };

const CInterfaceProxyVtbl * const _google_update_idl_ProxyVtblList[] = 
{
    ( CInterfaceProxyVtbl *) &_IJobObserverProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICoCreateAsyncStatusProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IAppVersionWebProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IAppCommandProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IProgressWndEventsProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IBrowserHttpRequest2ProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IApp2ProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IAppCommand2ProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IAppBundleProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IAppProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICoCreateAsyncProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IGoogleUpdateCoreProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IAppVersionProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IAppCommandWebProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IAppBundleWebProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IGoogleUpdate3WebProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IJobObserver2ProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IGoogleUpdate3WebSecurityProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IGoogleUpdate3ProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IRegistrationUpdateHookProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IPackageProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IProcessLauncher2ProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IAppWebProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICredentialDialogProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IProcessLauncherProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IOneClickProcessLauncherProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IGoogleUpdateProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICurrentStateProxyVtbl,
    0
};

const CInterfaceStubVtbl * const _google_update_idl_StubVtblList[] = 
{
    ( CInterfaceStubVtbl *) &_IJobObserverStubVtbl,
    ( CInterfaceStubVtbl *) &_ICoCreateAsyncStatusStubVtbl,
    ( CInterfaceStubVtbl *) &_IAppVersionWebStubVtbl,
    ( CInterfaceStubVtbl *) &_IAppCommandStubVtbl,
    ( CInterfaceStubVtbl *) &_IProgressWndEventsStubVtbl,
    ( CInterfaceStubVtbl *) &_IBrowserHttpRequest2StubVtbl,
    ( CInterfaceStubVtbl *) &_IApp2StubVtbl,
    ( CInterfaceStubVtbl *) &_IAppCommand2StubVtbl,
    ( CInterfaceStubVtbl *) &_IAppBundleStubVtbl,
    ( CInterfaceStubVtbl *) &_IAppStubVtbl,
    ( CInterfaceStubVtbl *) &_ICoCreateAsyncStubVtbl,
    ( CInterfaceStubVtbl *) &_IGoogleUpdateCoreStubVtbl,
    ( CInterfaceStubVtbl *) &_IAppVersionStubVtbl,
    ( CInterfaceStubVtbl *) &_IAppCommandWebStubVtbl,
    ( CInterfaceStubVtbl *) &_IAppBundleWebStubVtbl,
    ( CInterfaceStubVtbl *) &_IGoogleUpdate3WebStubVtbl,
    ( CInterfaceStubVtbl *) &_IJobObserver2StubVtbl,
    ( CInterfaceStubVtbl *) &_IGoogleUpdate3WebSecurityStubVtbl,
    ( CInterfaceStubVtbl *) &_IGoogleUpdate3StubVtbl,
    ( CInterfaceStubVtbl *) &_IRegistrationUpdateHookStubVtbl,
    ( CInterfaceStubVtbl *) &_IPackageStubVtbl,
    ( CInterfaceStubVtbl *) &_IProcessLauncher2StubVtbl,
    ( CInterfaceStubVtbl *) &_IAppWebStubVtbl,
    ( CInterfaceStubVtbl *) &_ICredentialDialogStubVtbl,
    ( CInterfaceStubVtbl *) &_IProcessLauncherStubVtbl,
    ( CInterfaceStubVtbl *) &_IOneClickProcessLauncherStubVtbl,
    ( CInterfaceStubVtbl *) &_IGoogleUpdateStubVtbl,
    ( CInterfaceStubVtbl *) &_ICurrentStateStubVtbl,
    0
};

PCInterfaceName const _google_update_idl_InterfaceNamesList[] = 
{
    "IJobObserver",
    "ICoCreateAsyncStatus",
    "IAppVersionWeb",
    "IAppCommand",
    "IProgressWndEvents",
    "IBrowserHttpRequest2",
    "IApp2",
    "IAppCommand2",
    "IAppBundle",
    "IApp",
    "ICoCreateAsync",
    "IGoogleUpdateCore",
    "IAppVersion",
    "IAppCommandWeb",
    "IAppBundleWeb",
    "IGoogleUpdate3Web",
    "IJobObserver2",
    "IGoogleUpdate3WebSecurity",
    "IGoogleUpdate3",
    "IRegistrationUpdateHook",
    "IPackage",
    "IProcessLauncher2",
    "IAppWeb",
    "ICredentialDialog",
    "IProcessLauncher",
    "IOneClickProcessLauncher",
    "IGoogleUpdate",
    "ICurrentState",
    0
};

const IID *  const _google_update_idl_BaseIIDList[] = 
{
    0,
    &IID_IDispatch,
    &IID_IDispatch,
    &IID_IDispatch,
    0,
    0,
    &IID_IDispatch,
    &IID_IDispatch,
    &IID_IDispatch,
    &IID_IDispatch,
    0,
    0,
    &IID_IDispatch,
    &IID_IDispatch,
    &IID_IDispatch,
    &IID_IDispatch,
    0,
    0,
    &IID_IDispatch,
    &IID_IDispatch,
    &IID_IDispatch,
    0,
    &IID_IDispatch,
    0,
    0,
    0,
    0,
    &IID_IDispatch,
    0
};


#define _google_update_idl_CHECK_IID(n)	IID_GENERIC_CHECK_IID( _google_update_idl, pIID, n)

int __stdcall _google_update_idl_IID_Lookup( const IID * pIID, int * pIndex )
{
    IID_BS_LOOKUP_SETUP

    IID_BS_LOOKUP_INITIAL_TEST( _google_update_idl, 28, 16 )
    IID_BS_LOOKUP_NEXT_TEST( _google_update_idl, 8 )
    IID_BS_LOOKUP_NEXT_TEST( _google_update_idl, 4 )
    IID_BS_LOOKUP_NEXT_TEST( _google_update_idl, 2 )
    IID_BS_LOOKUP_NEXT_TEST( _google_update_idl, 1 )
    IID_BS_LOOKUP_RETURN_RESULT( _google_update_idl, 28, *pIndex )
    
}

const ExtendedProxyFileInfo google_update_idl_ProxyFileInfo = 
{
    (PCInterfaceProxyVtblList *) & _google_update_idl_ProxyVtblList,
    (PCInterfaceStubVtblList *) & _google_update_idl_StubVtblList,
    (const PCInterfaceName * ) & _google_update_idl_InterfaceNamesList,
    (const IID ** ) & _google_update_idl_BaseIIDList,
    & _google_update_idl_IID_Lookup, 
    28,
    2,
    0, /* table of [async_uuid] interfaces */
    0, /* Filler1 */
    0, /* Filler2 */
    0  /* Filler3 */
};
#if _MSC_VER >= 1200
#pragma warning(pop)
#endif


#endif /* defined(_M_AMD64)*/

