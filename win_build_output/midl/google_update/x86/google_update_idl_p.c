

/* this ALWAYS GENERATED file contains the proxy stub code */


 /* File created by MIDL compiler version 8.xx.xxxx */
/* at a redacted point in time
 */
/* Compiler settings for ../../brave/chromium_src/google_update/google_update_idl.idl:
    Oicf, W1, Zp8, env=Win32 (32b run), target_arch=X86 8.xx.xxxx 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#if !defined(_M_IA64) && !defined(_M_AMD64) && !defined(_ARM_)


#pragma warning( disable: 4049 )  /* more than 64k source lines */
#if _MSC_VER >= 1200
#pragma warning(push)
#endif

#pragma warning( disable: 4211 )  /* redefine extern to static */
#pragma warning( disable: 4232 )  /* dllimport identity*/
#pragma warning( disable: 4024 )  /* array to pointer mapping*/
#pragma warning( disable: 4152 )  /* function/data pointer conversion in expression */
#pragma warning( disable: 4100 ) /* unreferenced arguments in x86 call */

#pragma optimize("", off ) 

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

#define TYPE_FORMAT_STRING_SIZE   1189                              
#define PROC_FORMAT_STRING_SIZE   5767                              
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

#if defined(_CONTROL_FLOW_GUARD_XFG)
#define XFG_TRAMPOLINES(ObjectType)\
static unsigned long ObjectType ## _UserSize_XFG(unsigned long * pFlags, unsigned long Offset, void * pObject)\
{\
return  ObjectType ## _UserSize(pFlags, Offset, pObject);\
}\
static unsigned char * ObjectType ## _UserMarshal_XFG(unsigned long * pFlags, unsigned char * pBuffer, void * pObject)\
{\
return ObjectType ## _UserMarshal(pFlags, pBuffer, pObject);\
}\
static unsigned char * ObjectType ## _UserUnmarshal_XFG(unsigned long * pFlags, unsigned char * pBuffer, void * pObject)\
{\
return ObjectType ## _UserUnmarshal(pFlags, pBuffer, pObject);\
}\
static void ObjectType ## _UserFree_XFG(unsigned long * pFlags, void * pObject)\
{\
ObjectType ## _UserFree(pFlags, pObject);\
}
#define XFG_TRAMPOLINES64(ObjectType)\
static unsigned long ObjectType ## _UserSize64_XFG(unsigned long * pFlags, unsigned long Offset, void * pObject)\
{\
return  ObjectType ## _UserSize64(pFlags, Offset, pObject);\
}\
static unsigned char * ObjectType ## _UserMarshal64_XFG(unsigned long * pFlags, unsigned char * pBuffer, void * pObject)\
{\
return ObjectType ## _UserMarshal64(pFlags, pBuffer, pObject);\
}\
static unsigned char * ObjectType ## _UserUnmarshal64_XFG(unsigned long * pFlags, unsigned char * pBuffer, void * pObject)\
{\
return ObjectType ## _UserUnmarshal64(pFlags, pBuffer, pObject);\
}\
static void ObjectType ## _UserFree64_XFG(unsigned long * pFlags, void * pObject)\
{\
ObjectType ## _UserFree64(pFlags, pObject);\
}
#define XFG_BIND_TRAMPOLINES(HandleType, ObjectType)\
static void* ObjectType ## _bind_XFG(HandleType pObject)\
{\
return ObjectType ## _bind((ObjectType) pObject);\
}\
static void ObjectType ## _unbind_XFG(HandleType pObject, handle_t ServerHandle)\
{\
ObjectType ## _unbind((ObjectType) pObject, ServerHandle);\
}
#define XFG_TRAMPOLINE_FPTR(Function) Function ## _XFG
#else
#define XFG_TRAMPOLINES(ObjectType)
#define XFG_TRAMPOLINES64(ObjectType)
#define XFG_BIND_TRAMPOLINES(HandleType, ObjectType)
#define XFG_TRAMPOLINE_FPTR(Function) Function
#endif


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


extern const MIDL_SERVER_INFO IPolicyStatus_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IPolicyStatus_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IPolicyStatusValue_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IPolicyStatusValue_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IPolicyStatus2_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IPolicyStatus2_ProxyInfo;


extern const MIDL_STUB_DESC Object_StubDesc;


extern const MIDL_SERVER_INFO IPolicyStatus3_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IPolicyStatus3_ProxyInfo;


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

#if !defined(__RPC_WIN32__)
#error  Invalid build platform for this stub.
#endif

#if !(TARGET_IS_NT50_OR_LATER)
#error You need Windows 2000 or later to run this stub because it uses these features:
#error   /robust command line switch.
#error However, your C/C++ compilation flags indicate you intend to run this app on earlier systems.
#error This app will fail with the RPC_X_WRONG_STUB_VERSION error.
#endif


static const google_update_idl_MIDL_PROC_FORMAT_STRING google_update_idl__MIDL_ProcFormatString =
    {
        0,
        {

	/* Procedure get_status */


	/* Procedure get_lastCheckPeriodMinutes */


	/* Procedure get_stateValue */


	/* Procedure get_Count */

			0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/*  2 */	NdrFcLong( 0x0 ),	/* 0 */
/*  6 */	NdrFcShort( 0x7 ),	/* 7 */
/*  8 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 10 */	NdrFcShort( 0x0 ),	/* 0 */
/* 12 */	NdrFcShort( 0x24 ),	/* 36 */
/* 14 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 16 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 18 */	NdrFcShort( 0x0 ),	/* 0 */
/* 20 */	NdrFcShort( 0x0 ),	/* 0 */
/* 22 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IAppCommandWeb0000 */


	/* Parameter minutes */


	/* Parameter __MIDL__ICurrentState0000 */


	/* Parameter count */

/* 24 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 26 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 28 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */


	/* Return value */


	/* Return value */


	/* Return value */

/* 30 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 32 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 34 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_Item */

/* 36 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 38 */	NdrFcLong( 0x0 ),	/* 0 */
/* 42 */	NdrFcShort( 0x8 ),	/* 8 */
/* 44 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 46 */	NdrFcShort( 0x8 ),	/* 8 */
/* 48 */	NdrFcShort( 0x8 ),	/* 8 */
/* 50 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x3,		/* 3 */
/* 52 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 54 */	NdrFcShort( 0x0 ),	/* 0 */
/* 56 */	NdrFcShort( 0x0 ),	/* 0 */
/* 58 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter index */

/* 60 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 62 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 64 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter bundle */

/* 66 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 68 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 70 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Return value */

/* 72 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 74 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 76 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_createdInstance */


	/* Procedure get_nextVersionWeb */


	/* Procedure createAppBundle */

/* 78 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 80 */	NdrFcLong( 0x0 ),	/* 0 */
/* 84 */	NdrFcShort( 0x9 ),	/* 9 */
/* 86 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 88 */	NdrFcShort( 0x0 ),	/* 0 */
/* 90 */	NdrFcShort( 0x8 ),	/* 8 */
/* 92 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 94 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 96 */	NdrFcShort( 0x0 ),	/* 0 */
/* 98 */	NdrFcShort( 0x0 ),	/* 0 */
/* 100 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter instance */


	/* Parameter next */


	/* Parameter app_bundle */

/* 102 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 104 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 106 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Return value */


	/* Return value */


	/* Return value */

/* 108 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 110 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 112 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_version */


	/* Procedure get_appId */


	/* Procedure get_updaterVersion */


	/* Procedure get_source */


	/* Procedure get_version */


	/* Procedure get_displayName */

/* 114 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 116 */	NdrFcLong( 0x0 ),	/* 0 */
/* 120 */	NdrFcShort( 0x7 ),	/* 7 */
/* 122 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 124 */	NdrFcShort( 0x0 ),	/* 0 */
/* 126 */	NdrFcShort( 0x8 ),	/* 8 */
/* 128 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 130 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 132 */	NdrFcShort( 0x1 ),	/* 1 */
/* 134 */	NdrFcShort( 0x0 ),	/* 0 */
/* 136 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IAppVersionWeb0000 */


	/* Parameter __MIDL__IAppWeb0000 */


	/* Parameter version */


	/* Parameter __MIDL__IPolicyStatusValue0000 */


	/* Parameter __MIDL__IAppVersion0000 */


	/* Parameter __MIDL__IAppBundle0000 */

/* 138 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 140 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 142 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Return value */


	/* Return value */


	/* Return value */


	/* Return value */


	/* Return value */


	/* Return value */

/* 144 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 146 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 148 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure createInstalledApp */


	/* Procedure put_displayName */

/* 150 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 152 */	NdrFcLong( 0x0 ),	/* 0 */
/* 156 */	NdrFcShort( 0x8 ),	/* 8 */
/* 158 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 160 */	NdrFcShort( 0x0 ),	/* 0 */
/* 162 */	NdrFcShort( 0x8 ),	/* 8 */
/* 164 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 166 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 168 */	NdrFcShort( 0x0 ),	/* 0 */
/* 170 */	NdrFcShort( 0x1 ),	/* 1 */
/* 172 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter app_id */


	/* Parameter __MIDL__IAppBundle0001 */

/* 174 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 176 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 178 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Return value */


	/* Return value */

/* 180 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 182 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 184 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_output */


	/* Procedure get_downloadPreferenceGroupPolicy */


	/* Procedure get_filename */


	/* Procedure get_appId */


	/* Procedure get_displayLanguage */

/* 186 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 188 */	NdrFcLong( 0x0 ),	/* 0 */
/* 192 */	NdrFcShort( 0x9 ),	/* 9 */
/* 194 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 196 */	NdrFcShort( 0x0 ),	/* 0 */
/* 198 */	NdrFcShort( 0x8 ),	/* 8 */
/* 200 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 202 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 204 */	NdrFcShort( 0x1 ),	/* 1 */
/* 206 */	NdrFcShort( 0x0 ),	/* 0 */
/* 208 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IAppCommandWeb0002 */


	/* Parameter pref */


	/* Parameter __MIDL__IPackage0001 */


	/* Parameter __MIDL__IApp0000 */


	/* Parameter __MIDL__IAppBundle0002 */

/* 210 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 212 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 214 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Return value */


	/* Return value */


	/* Return value */


	/* Return value */


	/* Return value */

/* 216 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 218 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 220 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_displayLanguage */

/* 222 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 224 */	NdrFcLong( 0x0 ),	/* 0 */
/* 228 */	NdrFcShort( 0xa ),	/* 10 */
/* 230 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 232 */	NdrFcShort( 0x0 ),	/* 0 */
/* 234 */	NdrFcShort( 0x8 ),	/* 8 */
/* 236 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 238 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 240 */	NdrFcShort( 0x0 ),	/* 0 */
/* 242 */	NdrFcShort( 0x1 ),	/* 1 */
/* 244 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IAppBundle0003 */

/* 246 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 248 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 250 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Return value */

/* 252 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 254 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 256 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_conflictValue */


	/* Procedure get_output */


	/* Procedure get_installSource */

/* 258 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 260 */	NdrFcLong( 0x0 ),	/* 0 */
/* 264 */	NdrFcShort( 0xb ),	/* 11 */
/* 266 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 268 */	NdrFcShort( 0x0 ),	/* 0 */
/* 270 */	NdrFcShort( 0x8 ),	/* 8 */
/* 272 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 274 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 276 */	NdrFcShort( 0x1 ),	/* 1 */
/* 278 */	NdrFcShort( 0x0 ),	/* 0 */
/* 280 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IPolicyStatusValue0003 */


	/* Parameter __MIDL__IAppCommand20000 */


	/* Parameter __MIDL__IAppBundle0004 */

/* 282 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 284 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 286 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Return value */


	/* Return value */


	/* Return value */

/* 288 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 290 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 292 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_installSource */

/* 294 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 296 */	NdrFcLong( 0x0 ),	/* 0 */
/* 300 */	NdrFcShort( 0xc ),	/* 12 */
/* 302 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 304 */	NdrFcShort( 0x0 ),	/* 0 */
/* 306 */	NdrFcShort( 0x8 ),	/* 8 */
/* 308 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 310 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 312 */	NdrFcShort( 0x0 ),	/* 0 */
/* 314 */	NdrFcShort( 0x1 ),	/* 1 */
/* 316 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IAppBundle0005 */

/* 318 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 320 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 322 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Return value */

/* 324 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 326 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 328 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_originURL */

/* 330 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 332 */	NdrFcLong( 0x0 ),	/* 0 */
/* 336 */	NdrFcShort( 0xd ),	/* 13 */
/* 338 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 340 */	NdrFcShort( 0x0 ),	/* 0 */
/* 342 */	NdrFcShort( 0x8 ),	/* 8 */
/* 344 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 346 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 348 */	NdrFcShort( 0x1 ),	/* 1 */
/* 350 */	NdrFcShort( 0x0 ),	/* 0 */
/* 352 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IAppBundle0006 */

/* 354 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 356 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 358 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Return value */

/* 360 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 362 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 364 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_originURL */

/* 366 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 368 */	NdrFcLong( 0x0 ),	/* 0 */
/* 372 */	NdrFcShort( 0xe ),	/* 14 */
/* 374 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 376 */	NdrFcShort( 0x0 ),	/* 0 */
/* 378 */	NdrFcShort( 0x8 ),	/* 8 */
/* 380 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 382 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 384 */	NdrFcShort( 0x0 ),	/* 0 */
/* 386 */	NdrFcShort( 0x1 ),	/* 1 */
/* 388 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IAppBundle0007 */

/* 390 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 392 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 394 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Return value */

/* 396 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 398 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 400 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_serverInstallDataIndex */


	/* Procedure get_offlineDirectory */

/* 402 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 404 */	NdrFcLong( 0x0 ),	/* 0 */
/* 408 */	NdrFcShort( 0xf ),	/* 15 */
/* 410 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 412 */	NdrFcShort( 0x0 ),	/* 0 */
/* 414 */	NdrFcShort( 0x8 ),	/* 8 */
/* 416 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 418 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 420 */	NdrFcShort( 0x1 ),	/* 1 */
/* 422 */	NdrFcShort( 0x0 ),	/* 0 */
/* 424 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IAppWeb0001 */


	/* Parameter offline_dir */

/* 426 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 428 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 430 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Return value */


	/* Return value */

/* 432 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 434 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 436 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_serverInstallDataIndex */


	/* Procedure put_offlineDirectory */

/* 438 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 440 */	NdrFcLong( 0x0 ),	/* 0 */
/* 444 */	NdrFcShort( 0x10 ),	/* 16 */
/* 446 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 448 */	NdrFcShort( 0x0 ),	/* 0 */
/* 450 */	NdrFcShort( 0x8 ),	/* 8 */
/* 452 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 454 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 456 */	NdrFcShort( 0x0 ),	/* 0 */
/* 458 */	NdrFcShort( 0x1 ),	/* 1 */
/* 460 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IAppWeb0002 */


	/* Parameter offline_dir */

/* 462 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 464 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 466 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Return value */


	/* Return value */

/* 468 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 470 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 472 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_sessionId */

/* 474 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 476 */	NdrFcLong( 0x0 ),	/* 0 */
/* 480 */	NdrFcShort( 0x11 ),	/* 17 */
/* 482 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 484 */	NdrFcShort( 0x0 ),	/* 0 */
/* 486 */	NdrFcShort( 0x8 ),	/* 8 */
/* 488 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 490 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 492 */	NdrFcShort( 0x1 ),	/* 1 */
/* 494 */	NdrFcShort( 0x0 ),	/* 0 */
/* 496 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter session_id */

/* 498 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 500 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 502 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Return value */

/* 504 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 506 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 508 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_sessionId */

/* 510 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 512 */	NdrFcLong( 0x0 ),	/* 0 */
/* 516 */	NdrFcShort( 0x12 ),	/* 18 */
/* 518 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 520 */	NdrFcShort( 0x0 ),	/* 0 */
/* 522 */	NdrFcShort( 0x8 ),	/* 8 */
/* 524 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 526 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 528 */	NdrFcShort( 0x0 ),	/* 0 */
/* 530 */	NdrFcShort( 0x1 ),	/* 1 */
/* 532 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter session_id */

/* 534 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 536 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 538 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Return value */

/* 540 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 542 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 544 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_sendPings */

/* 546 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 548 */	NdrFcLong( 0x0 ),	/* 0 */
/* 552 */	NdrFcShort( 0x13 ),	/* 19 */
/* 554 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 556 */	NdrFcShort( 0x0 ),	/* 0 */
/* 558 */	NdrFcShort( 0x22 ),	/* 34 */
/* 560 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 562 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 564 */	NdrFcShort( 0x0 ),	/* 0 */
/* 566 */	NdrFcShort( 0x0 ),	/* 0 */
/* 568 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter send_pings */

/* 570 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 572 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 574 */	0x6,		/* FC_SHORT */
			0x0,		/* 0 */

	/* Return value */

/* 576 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 578 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 580 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_sendPings */

/* 582 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 584 */	NdrFcLong( 0x0 ),	/* 0 */
/* 588 */	NdrFcShort( 0x14 ),	/* 20 */
/* 590 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 592 */	NdrFcShort( 0x6 ),	/* 6 */
/* 594 */	NdrFcShort( 0x8 ),	/* 8 */
/* 596 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 598 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 600 */	NdrFcShort( 0x0 ),	/* 0 */
/* 602 */	NdrFcShort( 0x0 ),	/* 0 */
/* 604 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter send_pings */

/* 606 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 608 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 610 */	0x6,		/* FC_SHORT */
			0x0,		/* 0 */

	/* Return value */

/* 612 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 614 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 616 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_priority */

/* 618 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 620 */	NdrFcLong( 0x0 ),	/* 0 */
/* 624 */	NdrFcShort( 0x15 ),	/* 21 */
/* 626 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 628 */	NdrFcShort( 0x0 ),	/* 0 */
/* 630 */	NdrFcShort( 0x24 ),	/* 36 */
/* 632 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 634 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 636 */	NdrFcShort( 0x0 ),	/* 0 */
/* 638 */	NdrFcShort( 0x0 ),	/* 0 */
/* 640 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter priority */

/* 642 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 644 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 646 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 648 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 650 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 652 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_priority */

/* 654 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 656 */	NdrFcLong( 0x0 ),	/* 0 */
/* 660 */	NdrFcShort( 0x16 ),	/* 22 */
/* 662 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 664 */	NdrFcShort( 0x8 ),	/* 8 */
/* 666 */	NdrFcShort( 0x8 ),	/* 8 */
/* 668 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 670 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 672 */	NdrFcShort( 0x0 ),	/* 0 */
/* 674 */	NdrFcShort( 0x0 ),	/* 0 */
/* 676 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter priority */

/* 678 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 680 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 682 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 684 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 686 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 688 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_postInstallAction */


	/* Procedure get_Count */

/* 690 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 692 */	NdrFcLong( 0x0 ),	/* 0 */
/* 696 */	NdrFcShort( 0x17 ),	/* 23 */
/* 698 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 700 */	NdrFcShort( 0x0 ),	/* 0 */
/* 702 */	NdrFcShort( 0x24 ),	/* 36 */
/* 704 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 706 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 708 */	NdrFcShort( 0x0 ),	/* 0 */
/* 710 */	NdrFcShort( 0x0 ),	/* 0 */
/* 712 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__ICurrentState0015 */


	/* Parameter count */

/* 714 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 716 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 718 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */


	/* Return value */

/* 720 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 722 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 724 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_Item */

/* 726 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 728 */	NdrFcLong( 0x0 ),	/* 0 */
/* 732 */	NdrFcShort( 0x18 ),	/* 24 */
/* 734 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 736 */	NdrFcShort( 0x8 ),	/* 8 */
/* 738 */	NdrFcShort( 0x8 ),	/* 8 */
/* 740 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x3,		/* 3 */
/* 742 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 744 */	NdrFcShort( 0x0 ),	/* 0 */
/* 746 */	NdrFcShort( 0x0 ),	/* 0 */
/* 748 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter index */

/* 750 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 752 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 754 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter app */

/* 756 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 758 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 760 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Return value */

/* 762 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 764 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 766 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_altTokens */

/* 768 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 770 */	NdrFcLong( 0x0 ),	/* 0 */
/* 774 */	NdrFcShort( 0x19 ),	/* 25 */
/* 776 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 778 */	NdrFcShort( 0x18 ),	/* 24 */
/* 780 */	NdrFcShort( 0x8 ),	/* 8 */
/* 782 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x4,		/* 4 */
/* 784 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 786 */	NdrFcShort( 0x0 ),	/* 0 */
/* 788 */	NdrFcShort( 0x0 ),	/* 0 */
/* 790 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter impersonation_token */

/* 792 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 794 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 796 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter primary_token */

/* 798 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 800 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 802 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter caller_proc_id */

/* 804 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 806 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 808 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 810 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 812 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 814 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_parentHWND */

/* 816 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 818 */	NdrFcLong( 0x0 ),	/* 0 */
/* 822 */	NdrFcShort( 0x1a ),	/* 26 */
/* 824 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 826 */	NdrFcShort( 0x8 ),	/* 8 */
/* 828 */	NdrFcShort( 0x8 ),	/* 8 */
/* 830 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 832 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 834 */	NdrFcShort( 0x0 ),	/* 0 */
/* 836 */	NdrFcShort( 0x0 ),	/* 0 */
/* 838 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter hwnd */

/* 840 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 842 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 844 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 846 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 848 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 850 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure initialize */

/* 852 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 854 */	NdrFcLong( 0x0 ),	/* 0 */
/* 858 */	NdrFcShort( 0x1b ),	/* 27 */
/* 860 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 862 */	NdrFcShort( 0x0 ),	/* 0 */
/* 864 */	NdrFcShort( 0x8 ),	/* 8 */
/* 866 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 868 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 870 */	NdrFcShort( 0x0 ),	/* 0 */
/* 872 */	NdrFcShort( 0x0 ),	/* 0 */
/* 874 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 876 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 878 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 880 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_command */


	/* Procedure createApp */

/* 882 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 884 */	NdrFcLong( 0x0 ),	/* 0 */
/* 888 */	NdrFcShort( 0x1c ),	/* 28 */
/* 890 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 892 */	NdrFcShort( 0x0 ),	/* 0 */
/* 894 */	NdrFcShort( 0x8 ),	/* 8 */
/* 896 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 898 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 900 */	NdrFcShort( 0x0 ),	/* 0 */
/* 902 */	NdrFcShort( 0x1 ),	/* 1 */
/* 904 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter command_id */


	/* Parameter app_id */

/* 906 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 908 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 910 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Parameter command */


	/* Parameter app */

/* 912 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 914 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 916 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Return value */


	/* Return value */

/* 918 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 920 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 922 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure createInstalledApp */

/* 924 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 926 */	NdrFcLong( 0x0 ),	/* 0 */
/* 930 */	NdrFcShort( 0x1d ),	/* 29 */
/* 932 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 934 */	NdrFcShort( 0x0 ),	/* 0 */
/* 936 */	NdrFcShort( 0x8 ),	/* 8 */
/* 938 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 940 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 942 */	NdrFcShort( 0x0 ),	/* 0 */
/* 944 */	NdrFcShort( 0x1 ),	/* 1 */
/* 946 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter app_id */

/* 948 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 950 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 952 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Parameter app */

/* 954 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 956 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 958 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Return value */

/* 960 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 962 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 964 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure createAllInstalledApps */

/* 966 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 968 */	NdrFcLong( 0x0 ),	/* 0 */
/* 972 */	NdrFcShort( 0x1e ),	/* 30 */
/* 974 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 976 */	NdrFcShort( 0x0 ),	/* 0 */
/* 978 */	NdrFcShort( 0x8 ),	/* 8 */
/* 980 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 982 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 984 */	NdrFcShort( 0x0 ),	/* 0 */
/* 986 */	NdrFcShort( 0x0 ),	/* 0 */
/* 988 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 990 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 992 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 994 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure checkForUpdate */

/* 996 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 998 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1002 */	NdrFcShort( 0x1f ),	/* 31 */
/* 1004 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1006 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1008 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1010 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 1012 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1014 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1016 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1018 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 1020 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1022 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1024 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure download */

/* 1026 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1028 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1032 */	NdrFcShort( 0x20 ),	/* 32 */
/* 1034 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1036 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1038 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1040 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 1042 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1044 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1046 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1048 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 1050 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1052 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1054 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure install */

/* 1056 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1058 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1062 */	NdrFcShort( 0x21 ),	/* 33 */
/* 1064 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1066 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1068 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1070 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 1072 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1074 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1076 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1078 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 1080 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1082 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1084 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure updateAllApps */

/* 1086 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1088 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1092 */	NdrFcShort( 0x22 ),	/* 34 */
/* 1094 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1096 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1098 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1100 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 1102 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1104 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1106 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1108 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 1110 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1112 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1114 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure stop */

/* 1116 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1118 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1122 */	NdrFcShort( 0x23 ),	/* 35 */
/* 1124 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1126 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1128 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1130 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 1132 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1134 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1136 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1138 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 1140 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1142 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1144 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure pause */

/* 1146 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1148 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1152 */	NdrFcShort( 0x24 ),	/* 36 */
/* 1154 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1156 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1158 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1160 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 1162 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1164 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1166 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1168 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 1170 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1172 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1174 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure resume */

/* 1176 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1178 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1182 */	NdrFcShort( 0x25 ),	/* 37 */
/* 1184 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1186 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1188 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1190 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 1192 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1194 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1196 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1198 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 1200 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1202 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1204 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure isBusy */

/* 1206 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1208 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1212 */	NdrFcShort( 0x26 ),	/* 38 */
/* 1214 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1216 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1218 */	NdrFcShort( 0x22 ),	/* 34 */
/* 1220 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 1222 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1224 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1226 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1228 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter is_busy */

/* 1230 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 1232 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1234 */	0x6,		/* FC_SHORT */
			0x0,		/* 0 */

	/* Return value */

/* 1236 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1238 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1240 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure downloadPackage */

/* 1242 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1244 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1248 */	NdrFcShort( 0x27 ),	/* 39 */
/* 1250 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 1252 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1254 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1256 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 1258 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 1260 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1262 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1264 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter app_id */

/* 1266 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 1268 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1270 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Parameter package_name */

/* 1272 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 1274 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1276 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Return value */

/* 1278 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1280 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1282 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_currentState */

/* 1284 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1286 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1290 */	NdrFcShort( 0x28 ),	/* 40 */
/* 1292 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1294 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1296 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1298 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 1300 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 1302 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1304 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1306 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter current_state */

/* 1308 */	NdrFcShort( 0x4113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=16 */
/* 1310 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1312 */	NdrFcShort( 0x42c ),	/* Type Offset=1068 */

	/* Return value */

/* 1314 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1316 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1318 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure createAppBundleWeb */


	/* Procedure get_currentVersion */

/* 1320 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1322 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1326 */	NdrFcShort( 0x7 ),	/* 7 */
/* 1328 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1330 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1332 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1334 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 1336 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1338 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1340 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1342 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter app_bundle_web */


	/* Parameter current */

/* 1344 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 1346 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1348 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Return value */


	/* Return value */

/* 1350 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1352 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1354 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_currentVersionWeb */


	/* Procedure get_nextVersion */

/* 1356 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1358 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1362 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1364 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1366 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1368 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1370 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 1372 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 1374 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1376 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1378 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter current */


	/* Parameter next */

/* 1380 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 1382 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1384 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Return value */


	/* Return value */

/* 1386 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1388 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1390 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_displayLanguage */


	/* Procedure get_conflictSource */


	/* Procedure get_displayName */

/* 1392 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1394 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1398 */	NdrFcShort( 0xa ),	/* 10 */
/* 1400 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1402 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1404 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1406 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 1408 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 1410 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1412 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1414 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IAppBundleWeb0000 */


	/* Parameter __MIDL__IPolicyStatusValue0002 */


	/* Parameter __MIDL__IApp0001 */

/* 1416 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 1418 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1420 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Return value */


	/* Return value */


	/* Return value */

/* 1422 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1424 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1426 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_displayLanguage */


	/* Procedure put_displayName */

/* 1428 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1430 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1434 */	NdrFcShort( 0xb ),	/* 11 */
/* 1436 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1438 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1440 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1442 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 1444 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 1446 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1448 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1450 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IAppBundleWeb0001 */


	/* Parameter __MIDL__IApp0002 */

/* 1452 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 1454 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1456 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Return value */


	/* Return value */

/* 1458 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1460 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1462 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_language */

/* 1464 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1466 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1470 */	NdrFcShort( 0xc ),	/* 12 */
/* 1472 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1474 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1476 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1478 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 1480 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 1482 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1484 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1486 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0003 */

/* 1488 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 1490 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1492 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Return value */

/* 1494 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1496 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1498 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_language */

/* 1500 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1502 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1506 */	NdrFcShort( 0xd ),	/* 13 */
/* 1508 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1510 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1512 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1514 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 1516 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 1518 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1520 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1522 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0004 */

/* 1524 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 1526 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1528 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Return value */

/* 1530 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1532 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1534 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_ap */

/* 1536 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1538 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1542 */	NdrFcShort( 0xe ),	/* 14 */
/* 1544 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1546 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1548 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1550 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 1552 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 1554 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1556 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1558 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0005 */

/* 1560 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 1562 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1564 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Return value */

/* 1566 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1568 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1570 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_ap */

/* 1572 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1574 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1578 */	NdrFcShort( 0xf ),	/* 15 */
/* 1580 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1582 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1584 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1586 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 1588 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 1590 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1592 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1594 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0006 */

/* 1596 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 1598 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1600 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Return value */

/* 1602 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1604 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1606 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_ttToken */

/* 1608 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1610 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1614 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1616 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1618 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1620 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1622 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 1624 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 1626 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1628 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1630 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0007 */

/* 1632 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 1634 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1636 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Return value */

/* 1638 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1640 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1642 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_ttToken */

/* 1644 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1646 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1650 */	NdrFcShort( 0x11 ),	/* 17 */
/* 1652 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1654 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1656 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1658 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 1660 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 1662 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1664 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1666 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0008 */

/* 1668 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 1670 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1672 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Return value */

/* 1674 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1676 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1678 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_completionMessage */


	/* Procedure get_iid */

/* 1680 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1682 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1686 */	NdrFcShort( 0x12 ),	/* 18 */
/* 1688 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1690 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1692 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1694 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 1696 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 1698 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1700 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1702 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__ICurrentState0010 */


	/* Parameter __MIDL__IApp0009 */

/* 1704 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 1706 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1708 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Return value */


	/* Return value */

/* 1710 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1712 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1714 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_iid */

/* 1716 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1718 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1722 */	NdrFcShort( 0x13 ),	/* 19 */
/* 1724 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1726 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1728 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1730 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 1732 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 1734 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1736 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1738 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0010 */

/* 1740 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 1742 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1744 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Return value */

/* 1746 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1748 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1750 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_brandCode */

/* 1752 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1754 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1758 */	NdrFcShort( 0x14 ),	/* 20 */
/* 1760 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1762 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1764 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1766 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 1768 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 1770 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1772 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1774 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0011 */

/* 1776 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 1778 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1780 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Return value */

/* 1782 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1784 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1786 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_brandCode */

/* 1788 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1790 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1794 */	NdrFcShort( 0x15 ),	/* 21 */
/* 1796 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1798 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1800 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1802 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 1804 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 1806 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1808 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1810 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0012 */

/* 1812 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 1814 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1816 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Return value */

/* 1818 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1820 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1822 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_postInstallUrl */


	/* Procedure get_clientId */

/* 1824 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1826 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1830 */	NdrFcShort( 0x16 ),	/* 22 */
/* 1832 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1834 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1836 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1838 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 1840 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 1842 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1844 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1846 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__ICurrentState0014 */


	/* Parameter __MIDL__IApp0013 */

/* 1848 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 1850 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1852 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Return value */


	/* Return value */

/* 1854 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1856 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1858 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_clientId */

/* 1860 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1862 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1866 */	NdrFcShort( 0x17 ),	/* 23 */
/* 1868 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1870 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1872 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1874 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 1876 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 1878 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1880 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1882 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0014 */

/* 1884 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 1886 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1888 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Return value */

/* 1890 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1892 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1894 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_labels */

/* 1896 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1898 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1902 */	NdrFcShort( 0x18 ),	/* 24 */
/* 1904 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1906 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1908 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1910 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 1912 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 1914 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1916 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1918 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0015 */

/* 1920 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 1922 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1924 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Return value */

/* 1926 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1928 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1930 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_labels */

/* 1932 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1934 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1938 */	NdrFcShort( 0x19 ),	/* 25 */
/* 1940 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1942 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1944 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1946 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 1948 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 1950 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1952 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1954 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0016 */

/* 1956 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 1958 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1960 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Return value */

/* 1962 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 1964 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 1966 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_referralId */

/* 1968 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 1970 */	NdrFcLong( 0x0 ),	/* 0 */
/* 1974 */	NdrFcShort( 0x1a ),	/* 26 */
/* 1976 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 1978 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1980 */	NdrFcShort( 0x8 ),	/* 8 */
/* 1982 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 1984 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 1986 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1988 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1990 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0017 */

/* 1992 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 1994 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 1996 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Return value */

/* 1998 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2000 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2002 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_referralId */

/* 2004 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2006 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2010 */	NdrFcShort( 0x1b ),	/* 27 */
/* 2012 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2014 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2016 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2018 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 2020 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 2022 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2024 */	NdrFcShort( 0x1 ),	/* 1 */
/* 2026 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0018 */

/* 2028 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 2030 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2032 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Return value */

/* 2034 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2036 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2038 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_browserType */

/* 2040 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2042 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2046 */	NdrFcShort( 0x1d ),	/* 29 */
/* 2048 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2050 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2052 */	NdrFcShort( 0x24 ),	/* 36 */
/* 2054 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 2056 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2058 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2060 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2062 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0019 */

/* 2064 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2066 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2068 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2070 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2072 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2074 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_browserType */

/* 2076 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2078 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2082 */	NdrFcShort( 0x1e ),	/* 30 */
/* 2084 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2086 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2088 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2090 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 2092 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2094 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2096 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2098 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0020 */

/* 2100 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2102 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2104 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2106 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2108 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2110 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_clientInstallData */

/* 2112 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2114 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2118 */	NdrFcShort( 0x1f ),	/* 31 */
/* 2120 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2122 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2124 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2126 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 2128 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 2130 */	NdrFcShort( 0x1 ),	/* 1 */
/* 2132 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2134 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0021 */

/* 2136 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 2138 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2140 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Return value */

/* 2142 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2144 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2146 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_clientInstallData */

/* 2148 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2150 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2154 */	NdrFcShort( 0x20 ),	/* 32 */
/* 2156 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2158 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2160 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2162 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 2164 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 2166 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2168 */	NdrFcShort( 0x1 ),	/* 1 */
/* 2170 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0022 */

/* 2172 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 2174 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2176 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Return value */

/* 2178 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2180 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2182 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_serverInstallDataIndex */

/* 2184 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2186 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2190 */	NdrFcShort( 0x21 ),	/* 33 */
/* 2192 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2194 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2196 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2198 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 2200 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 2202 */	NdrFcShort( 0x1 ),	/* 1 */
/* 2204 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2206 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0023 */

/* 2208 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 2210 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2212 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Return value */

/* 2214 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2216 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2218 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_serverInstallDataIndex */

/* 2220 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2222 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2226 */	NdrFcShort( 0x22 ),	/* 34 */
/* 2228 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2230 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2232 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2234 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 2236 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 2238 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2240 */	NdrFcShort( 0x1 ),	/* 1 */
/* 2242 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0024 */

/* 2244 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 2246 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2248 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Return value */

/* 2250 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2252 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2254 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_isEulaAccepted */

/* 2256 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2258 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2262 */	NdrFcShort( 0x23 ),	/* 35 */
/* 2264 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2266 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2268 */	NdrFcShort( 0x22 ),	/* 34 */
/* 2270 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 2272 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2274 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2276 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2278 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0025 */

/* 2280 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2282 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2284 */	0x6,		/* FC_SHORT */
			0x0,		/* 0 */

	/* Return value */

/* 2286 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2288 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2290 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_isEulaAccepted */

/* 2292 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2294 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2298 */	NdrFcShort( 0x24 ),	/* 36 */
/* 2300 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2302 */	NdrFcShort( 0x6 ),	/* 6 */
/* 2304 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2306 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 2308 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2310 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2312 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2314 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0026 */

/* 2316 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2318 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2320 */	0x6,		/* FC_SHORT */
			0x0,		/* 0 */

	/* Return value */

/* 2322 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2324 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2326 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_usageStatsEnable */

/* 2328 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2330 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2334 */	NdrFcShort( 0x25 ),	/* 37 */
/* 2336 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2338 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2340 */	NdrFcShort( 0x24 ),	/* 36 */
/* 2342 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 2344 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2346 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2348 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2350 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0027 */

/* 2352 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2354 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2356 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2358 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2360 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2362 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_usageStatsEnable */

/* 2364 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2366 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2370 */	NdrFcShort( 0x26 ),	/* 38 */
/* 2372 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2374 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2376 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2378 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 2380 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2382 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2384 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2386 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0028 */

/* 2388 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2390 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2392 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2394 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2396 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2398 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_installTimeDiffSec */

/* 2400 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2402 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2406 */	NdrFcShort( 0x27 ),	/* 39 */
/* 2408 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2410 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2412 */	NdrFcShort( 0x24 ),	/* 36 */
/* 2414 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 2416 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2418 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2420 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2422 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0029 */

/* 2424 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2426 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2428 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 2430 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2432 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2434 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_currentState */

/* 2436 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2438 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2442 */	NdrFcShort( 0x28 ),	/* 40 */
/* 2444 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2446 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2448 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2450 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 2452 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2454 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2456 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2458 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp0030 */

/* 2460 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 2462 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2464 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Return value */

/* 2466 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2468 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2470 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_untrustedData */

/* 2472 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2474 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2478 */	NdrFcShort( 0x29 ),	/* 41 */
/* 2480 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2482 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2484 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2486 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 2488 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 2490 */	NdrFcShort( 0x1 ),	/* 1 */
/* 2492 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2494 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp20000 */

/* 2496 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 2498 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2500 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Return value */

/* 2502 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2504 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2506 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_untrustedData */

/* 2508 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2510 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2514 */	NdrFcShort( 0x2a ),	/* 42 */
/* 2516 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2518 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2520 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2522 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 2524 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 2526 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2528 */	NdrFcShort( 0x1 ),	/* 1 */
/* 2530 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IApp20001 */

/* 2532 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 2534 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2536 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Return value */

/* 2538 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2540 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2542 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_isDone */


	/* Procedure get_isWebAccessible */

/* 2544 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2546 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2550 */	NdrFcShort( 0x7 ),	/* 7 */
/* 2552 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2554 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2556 */	NdrFcShort( 0x22 ),	/* 34 */
/* 2558 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 2560 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2562 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2564 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2566 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter is_done */


	/* Parameter __MIDL__IAppCommand0000 */

/* 2568 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2570 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2572 */	0x6,		/* FC_SHORT */
			0x0,		/* 0 */

	/* Return value */


	/* Return value */

/* 2574 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2576 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2578 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_completionHResult */


	/* Procedure get_packageCount */


	/* Procedure get_exitCode */


	/* Procedure get_packageCount */


	/* Procedure get_status */

/* 2580 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2582 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2586 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2588 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2590 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2592 */	NdrFcShort( 0x24 ),	/* 36 */
/* 2594 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 2596 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2598 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2600 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2602 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter hr */


	/* Parameter count */


	/* Parameter __MIDL__IAppCommandWeb0001 */


	/* Parameter count */


	/* Parameter __MIDL__IAppCommand0001 */

/* 2604 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2606 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2608 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */


	/* Return value */


	/* Return value */


	/* Return value */


	/* Return value */

/* 2610 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2612 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2614 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_bytesDownloaded */


	/* Procedure get_exitCode */

/* 2616 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2618 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2622 */	NdrFcShort( 0x9 ),	/* 9 */
/* 2624 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2626 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2628 */	NdrFcShort( 0x24 ),	/* 36 */
/* 2630 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 2632 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2634 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2636 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2638 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__ICurrentState0002 */


	/* Parameter __MIDL__IAppCommand0002 */

/* 2640 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2642 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2644 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */


	/* Return value */

/* 2646 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2648 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2650 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure execute */


	/* Procedure execute */

/* 2652 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2654 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2658 */	NdrFcShort( 0xa ),	/* 10 */
/* 2660 */	NdrFcShort( 0x98 ),	/* x86 Stack size/offset = 152 */
/* 2662 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2664 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2666 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0xa,		/* 10 */
/* 2668 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 2670 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2672 */	NdrFcShort( 0x1 ),	/* 1 */
/* 2674 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter arg1 */


	/* Parameter arg1 */

/* 2676 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 2678 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2680 */	NdrFcShort( 0x43a ),	/* Type Offset=1082 */

	/* Parameter arg2 */


	/* Parameter arg2 */

/* 2682 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 2684 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 2686 */	NdrFcShort( 0x43a ),	/* Type Offset=1082 */

	/* Parameter arg3 */


	/* Parameter arg3 */

/* 2688 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 2690 */	NdrFcShort( 0x24 ),	/* x86 Stack size/offset = 36 */
/* 2692 */	NdrFcShort( 0x43a ),	/* Type Offset=1082 */

	/* Parameter arg4 */


	/* Parameter arg4 */

/* 2694 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 2696 */	NdrFcShort( 0x34 ),	/* x86 Stack size/offset = 52 */
/* 2698 */	NdrFcShort( 0x43a ),	/* Type Offset=1082 */

	/* Parameter arg5 */


	/* Parameter arg5 */

/* 2700 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 2702 */	NdrFcShort( 0x44 ),	/* x86 Stack size/offset = 68 */
/* 2704 */	NdrFcShort( 0x43a ),	/* Type Offset=1082 */

	/* Parameter arg6 */


	/* Parameter arg6 */

/* 2706 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 2708 */	NdrFcShort( 0x54 ),	/* x86 Stack size/offset = 84 */
/* 2710 */	NdrFcShort( 0x43a ),	/* Type Offset=1082 */

	/* Parameter arg7 */


	/* Parameter arg7 */

/* 2712 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 2714 */	NdrFcShort( 0x64 ),	/* x86 Stack size/offset = 100 */
/* 2716 */	NdrFcShort( 0x43a ),	/* Type Offset=1082 */

	/* Parameter arg8 */


	/* Parameter arg8 */

/* 2718 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 2720 */	NdrFcShort( 0x74 ),	/* x86 Stack size/offset = 116 */
/* 2722 */	NdrFcShort( 0x43a ),	/* Type Offset=1082 */

	/* Parameter arg9 */


	/* Parameter arg9 */

/* 2724 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 2726 */	NdrFcShort( 0x84 ),	/* x86 Stack size/offset = 132 */
/* 2728 */	NdrFcShort( 0x43a ),	/* Type Offset=1082 */

	/* Return value */


	/* Return value */

/* 2730 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2732 */	NdrFcShort( 0x94 ),	/* x86 Stack size/offset = 148 */
/* 2734 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_packageWeb */


	/* Procedure get_package */

/* 2736 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2738 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2742 */	NdrFcShort( 0x9 ),	/* 9 */
/* 2744 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 2746 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2748 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2750 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x3,		/* 3 */
/* 2752 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2754 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2756 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2758 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter index */


	/* Parameter index */

/* 2760 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 2762 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2764 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter package */


	/* Parameter package */

/* 2766 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 2768 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2770 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Return value */


	/* Return value */

/* 2772 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2774 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2776 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get */

/* 2778 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2780 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2784 */	NdrFcShort( 0x7 ),	/* 7 */
/* 2786 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2788 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2790 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2792 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 2794 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 2796 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2798 */	NdrFcShort( 0x1 ),	/* 1 */
/* 2800 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter dir */

/* 2802 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 2804 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2806 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Return value */

/* 2808 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2810 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2812 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_isAvailable */

/* 2814 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2816 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2820 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2822 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2824 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2826 */	NdrFcShort( 0x22 ),	/* 34 */
/* 2828 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 2830 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2832 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2834 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2836 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IPackage0000 */

/* 2838 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2840 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2842 */	0x6,		/* FC_SHORT */
			0x0,		/* 0 */

	/* Return value */

/* 2844 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2846 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2848 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_value */


	/* Procedure get_availableVersion */

/* 2850 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2852 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2856 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2858 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2860 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2862 */	NdrFcShort( 0x8 ),	/* 8 */
/* 2864 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 2866 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 2868 */	NdrFcShort( 0x1 ),	/* 1 */
/* 2870 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2872 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__IPolicyStatusValue0001 */


	/* Parameter __MIDL__ICurrentState0001 */

/* 2874 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 2876 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2878 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Return value */


	/* Return value */

/* 2880 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2882 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2884 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_packageCacheSizeLimitMBytes */


	/* Procedure get_totalBytesToDownload */

/* 2886 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2888 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2892 */	NdrFcShort( 0xa ),	/* 10 */
/* 2894 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2896 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2898 */	NdrFcShort( 0x24 ),	/* 36 */
/* 2900 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 2902 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2904 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2906 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2908 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter limit */


	/* Parameter __MIDL__ICurrentState0003 */

/* 2910 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2912 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2914 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */


	/* Return value */

/* 2916 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2918 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2920 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_packageCacheExpirationTimeDays */


	/* Procedure get_downloadTimeRemainingMs */

/* 2922 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2924 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2928 */	NdrFcShort( 0xb ),	/* 11 */
/* 2930 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2932 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2934 */	NdrFcShort( 0x24 ),	/* 36 */
/* 2936 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 2938 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2940 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2942 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2944 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter days */


	/* Parameter __MIDL__ICurrentState0004 */

/* 2946 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2948 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2950 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */


	/* Return value */

/* 2952 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2954 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2956 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_nextRetryTime */

/* 2958 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2960 */	NdrFcLong( 0x0 ),	/* 0 */
/* 2964 */	NdrFcShort( 0xc ),	/* 12 */
/* 2966 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 2968 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2970 */	NdrFcShort( 0x2c ),	/* 44 */
/* 2972 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 2974 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 2976 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2978 */	NdrFcShort( 0x0 ),	/* 0 */
/* 2980 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__ICurrentState0005 */

/* 2982 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 2984 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 2986 */	0xb,		/* FC_HYPER */
			0x0,		/* 0 */

	/* Return value */

/* 2988 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 2990 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 2992 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_length */


	/* Procedure get_installProgress */

/* 2994 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 2996 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3000 */	NdrFcShort( 0xd ),	/* 13 */
/* 3002 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3004 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3006 */	NdrFcShort( 0x24 ),	/* 36 */
/* 3008 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 3010 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 3012 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3014 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3016 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter index */


	/* Parameter __MIDL__ICurrentState0006 */

/* 3018 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 3020 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3022 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */


	/* Return value */

/* 3024 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3026 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3028 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_installTimeRemainingMs */

/* 3030 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3032 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3036 */	NdrFcShort( 0xe ),	/* 14 */
/* 3038 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3040 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3042 */	NdrFcShort( 0x24 ),	/* 36 */
/* 3044 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 3046 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 3048 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3050 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3052 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__ICurrentState0007 */

/* 3054 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 3056 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3058 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 3060 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3062 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3064 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_isCanceled */

/* 3066 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3068 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3072 */	NdrFcShort( 0xf ),	/* 15 */
/* 3074 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3076 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3078 */	NdrFcShort( 0x22 ),	/* 34 */
/* 3080 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 3082 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 3084 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3086 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3088 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter is_canceled */

/* 3090 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 3092 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3094 */	0x6,		/* FC_SHORT */
			0x0,		/* 0 */

	/* Return value */

/* 3096 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3098 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3100 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_errorCode */

/* 3102 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3104 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3108 */	NdrFcShort( 0x10 ),	/* 16 */
/* 3110 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3112 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3114 */	NdrFcShort( 0x24 ),	/* 36 */
/* 3116 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 3118 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 3120 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3122 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3124 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__ICurrentState0008 */

/* 3126 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 3128 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3130 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 3132 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3134 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3136 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_extraCode1 */

/* 3138 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3140 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3144 */	NdrFcShort( 0x11 ),	/* 17 */
/* 3146 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3148 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3150 */	NdrFcShort( 0x24 ),	/* 36 */
/* 3152 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 3154 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 3156 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3158 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3160 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__ICurrentState0009 */

/* 3162 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 3164 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3166 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 3168 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3170 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3172 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_installerResultCode */

/* 3174 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3176 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3180 */	NdrFcShort( 0x13 ),	/* 19 */
/* 3182 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3184 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3186 */	NdrFcShort( 0x24 ),	/* 36 */
/* 3188 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 3190 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 3192 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3194 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3196 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__ICurrentState0011 */

/* 3198 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 3200 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3202 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 3204 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3206 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3208 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_installerResultExtraCode1 */

/* 3210 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3212 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3216 */	NdrFcShort( 0x14 ),	/* 20 */
/* 3218 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3220 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3222 */	NdrFcShort( 0x24 ),	/* 36 */
/* 3224 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 3226 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 3228 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3230 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3232 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__ICurrentState0012 */

/* 3234 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 3236 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3238 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 3240 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3242 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3244 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_postInstallLaunchCommandLine */

/* 3246 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3248 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3252 */	NdrFcShort( 0x15 ),	/* 21 */
/* 3254 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3256 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3258 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3260 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 3262 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 3264 */	NdrFcShort( 0x1 ),	/* 1 */
/* 3266 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3268 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter __MIDL__ICurrentState0013 */

/* 3270 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 3272 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3274 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Return value */

/* 3276 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3278 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3280 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure UpdateRegistry */

/* 3282 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3284 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3288 */	NdrFcShort( 0x7 ),	/* 7 */
/* 3290 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 3292 */	NdrFcShort( 0x6 ),	/* 6 */
/* 3294 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3296 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 3298 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 3300 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3302 */	NdrFcShort( 0x1 ),	/* 1 */
/* 3304 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter app_id */

/* 3306 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 3308 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3310 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Parameter is_machine */

/* 3312 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 3314 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3316 */	0x6,		/* FC_SHORT */
			0x0,		/* 0 */

	/* Return value */

/* 3318 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3320 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3322 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure QueryUserForCredentials */

/* 3324 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3326 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3330 */	NdrFcShort( 0x3 ),	/* 3 */
/* 3332 */	NdrFcShort( 0x1c ),	/* x86 Stack size/offset = 28 */
/* 3334 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3336 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3338 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x6,		/* 6 */
/* 3340 */	0x8,		/* 8 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 3342 */	NdrFcShort( 0x1 ),	/* 1 */
/* 3344 */	NdrFcShort( 0x1 ),	/* 1 */
/* 3346 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter owner_hwnd */

/* 3348 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 3350 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3352 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter server */

/* 3354 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 3356 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3358 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Parameter message */

/* 3360 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 3362 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3364 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Parameter username */

/* 3366 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 3368 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 3370 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Parameter password */

/* 3372 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 3374 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 3376 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Return value */

/* 3378 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3380 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 3382 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_updatesSuppressedTimes */

/* 3384 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3386 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3390 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3392 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 3394 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3396 */	NdrFcShort( 0x76 ),	/* 118 */
/* 3398 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x5,		/* 5 */
/* 3400 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 3402 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3404 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3406 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter start_hour */

/* 3408 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 3410 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3412 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter start_min */

/* 3414 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 3416 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3418 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter duration_min */

/* 3420 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 3422 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3424 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter are_updates_suppressed */

/* 3426 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 3428 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 3430 */	0x6,		/* FC_SHORT */
			0x0,		/* 0 */

	/* Return value */

/* 3432 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3434 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 3436 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_effectivePolicyForAppInstalls */

/* 3438 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3440 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3444 */	NdrFcShort( 0xc ),	/* 12 */
/* 3446 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 3448 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3450 */	NdrFcShort( 0x24 ),	/* 36 */
/* 3452 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 3454 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 3456 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3458 */	NdrFcShort( 0x1 ),	/* 1 */
/* 3460 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter app_id */

/* 3462 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 3464 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3466 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Parameter policy */

/* 3468 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 3470 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3472 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 3474 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3476 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3478 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_effectivePolicyForAppUpdates */

/* 3480 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3482 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3486 */	NdrFcShort( 0xd ),	/* 13 */
/* 3488 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 3490 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3492 */	NdrFcShort( 0x24 ),	/* 36 */
/* 3494 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 3496 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 3498 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3500 */	NdrFcShort( 0x1 ),	/* 1 */
/* 3502 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter app_id */

/* 3504 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 3506 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3508 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Parameter policy */

/* 3510 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 3512 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3514 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 3516 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3518 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3520 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_targetVersionPrefix */

/* 3522 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3524 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3528 */	NdrFcShort( 0xe ),	/* 14 */
/* 3530 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 3532 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3534 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3536 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 3538 */	0x8,		/* 8 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 3540 */	NdrFcShort( 0x1 ),	/* 1 */
/* 3542 */	NdrFcShort( 0x1 ),	/* 1 */
/* 3544 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter app_id */

/* 3546 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 3548 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3550 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Parameter prefix */

/* 3552 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 3554 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3556 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Return value */

/* 3558 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3560 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3562 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_isRollbackToTargetVersionAllowed */

/* 3564 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3566 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3570 */	NdrFcShort( 0xf ),	/* 15 */
/* 3572 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 3574 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3576 */	NdrFcShort( 0x22 ),	/* 34 */
/* 3578 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 3580 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 3582 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3584 */	NdrFcShort( 0x1 ),	/* 1 */
/* 3586 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter app_id */

/* 3588 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 3590 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3592 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Parameter rollback_allowed */

/* 3594 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 3596 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3598 */	0x6,		/* FC_SHORT */
			0x0,		/* 0 */

	/* Return value */

/* 3600 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3602 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3604 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_hasConflict */

/* 3606 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3608 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3612 */	NdrFcShort( 0x9 ),	/* 9 */
/* 3614 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3616 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3618 */	NdrFcShort( 0x22 ),	/* 34 */
/* 3620 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 3622 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 3624 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3626 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3628 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter has_conflict */

/* 3630 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 3632 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3634 */	0x6,		/* FC_SHORT */
			0x0,		/* 0 */

	/* Return value */

/* 3636 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3638 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3640 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_lastCheckedTime */

/* 3642 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3644 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3648 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3650 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3652 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3654 */	NdrFcShort( 0x2c ),	/* 44 */
/* 3656 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 3658 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 3660 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3662 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3664 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter last_checked */

/* 3666 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 3668 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3670 */	0xc,		/* FC_DOUBLE */
			0x0,		/* 0 */

	/* Return value */

/* 3672 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3674 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3676 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure OnInstalling */


	/* Procedure createAllInstalledApps */


	/* Procedure refreshPolicies */

/* 3678 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3680 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3684 */	NdrFcShort( 0x9 ),	/* 9 */
/* 3686 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3688 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3690 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3692 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 3694 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 3696 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3698 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3700 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */


	/* Return value */


	/* Return value */

/* 3702 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3704 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3706 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_lastCheckPeriodMinutes */

/* 3708 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3710 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3714 */	NdrFcShort( 0xa ),	/* 10 */
/* 3716 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3718 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3720 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3722 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 3724 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 3726 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3728 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3730 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter value */

/* 3732 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 3734 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3736 */	NdrFcShort( 0x44c ),	/* Type Offset=1100 */

	/* Return value */

/* 3738 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3740 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3742 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_updatesSuppressedTimes */

/* 3744 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3746 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3750 */	NdrFcShort( 0xb ),	/* 11 */
/* 3752 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 3754 */	NdrFcShort( 0x1a ),	/* 26 */
/* 3756 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3758 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x3,		/* 3 */
/* 3760 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 3762 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3764 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3766 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter value */

/* 3768 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 3770 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3772 */	NdrFcShort( 0x44c ),	/* Type Offset=1100 */

	/* Parameter are_updates_suppressed */

/* 3774 */	NdrFcShort( 0x148 ),	/* Flags:  in, base type, simple ref, */
/* 3776 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3778 */	0x6,		/* FC_SHORT */
			0x0,		/* 0 */

	/* Return value */

/* 3780 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3782 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3784 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_downloadPreferenceGroupPolicy */

/* 3786 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3788 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3792 */	NdrFcShort( 0xc ),	/* 12 */
/* 3794 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3796 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3798 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3800 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 3802 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 3804 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3806 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3808 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter value */

/* 3810 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 3812 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3814 */	NdrFcShort( 0x44c ),	/* Type Offset=1100 */

	/* Return value */

/* 3816 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3818 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3820 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_packageCacheSizeLimitMBytes */

/* 3822 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3824 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3828 */	NdrFcShort( 0xd ),	/* 13 */
/* 3830 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3832 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3834 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3836 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 3838 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 3840 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3842 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3844 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter value */

/* 3846 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 3848 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3850 */	NdrFcShort( 0x44c ),	/* Type Offset=1100 */

	/* Return value */

/* 3852 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3854 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3856 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_packageCacheExpirationTimeDays */

/* 3858 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3860 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3864 */	NdrFcShort( 0xe ),	/* 14 */
/* 3866 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3868 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3870 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3872 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 3874 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 3876 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3878 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3880 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter value */

/* 3882 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 3884 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3886 */	NdrFcShort( 0x44c ),	/* Type Offset=1100 */

	/* Return value */

/* 3888 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3890 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3892 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_proxyMode */

/* 3894 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3896 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3900 */	NdrFcShort( 0xf ),	/* 15 */
/* 3902 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3904 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3906 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3908 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 3910 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 3912 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3914 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3916 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter value */

/* 3918 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 3920 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3922 */	NdrFcShort( 0x44c ),	/* Type Offset=1100 */

	/* Return value */

/* 3924 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3926 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3928 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_proxyPacUrl */

/* 3930 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3932 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3936 */	NdrFcShort( 0x10 ),	/* 16 */
/* 3938 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3940 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3942 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3944 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 3946 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 3948 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3950 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3952 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter value */

/* 3954 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 3956 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3958 */	NdrFcShort( 0x44c ),	/* Type Offset=1100 */

	/* Return value */

/* 3960 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3962 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 3964 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_proxyServer */

/* 3966 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 3968 */	NdrFcLong( 0x0 ),	/* 0 */
/* 3972 */	NdrFcShort( 0x11 ),	/* 17 */
/* 3974 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 3976 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3978 */	NdrFcShort( 0x8 ),	/* 8 */
/* 3980 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 3982 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 3984 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3986 */	NdrFcShort( 0x0 ),	/* 0 */
/* 3988 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter value */

/* 3990 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 3992 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 3994 */	NdrFcShort( 0x44c ),	/* Type Offset=1100 */

	/* Return value */

/* 3996 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 3998 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4000 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_effectivePolicyForAppInstalls */

/* 4002 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4004 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4008 */	NdrFcShort( 0x12 ),	/* 18 */
/* 4010 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 4012 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4014 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4016 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 4018 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 4020 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4022 */	NdrFcShort( 0x1 ),	/* 1 */
/* 4024 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter app_id */

/* 4026 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 4028 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4030 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Parameter value */

/* 4032 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 4034 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4036 */	NdrFcShort( 0x44c ),	/* Type Offset=1100 */

	/* Return value */

/* 4038 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4040 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4042 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_effectivePolicyForAppUpdates */

/* 4044 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4046 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4050 */	NdrFcShort( 0x13 ),	/* 19 */
/* 4052 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 4054 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4056 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4058 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 4060 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 4062 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4064 */	NdrFcShort( 0x1 ),	/* 1 */
/* 4066 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter app_id */

/* 4068 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 4070 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4072 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Parameter value */

/* 4074 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 4076 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4078 */	NdrFcShort( 0x44c ),	/* Type Offset=1100 */

	/* Return value */

/* 4080 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4082 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4084 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_targetVersionPrefix */

/* 4086 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4088 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4092 */	NdrFcShort( 0x14 ),	/* 20 */
/* 4094 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 4096 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4098 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4100 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 4102 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 4104 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4106 */	NdrFcShort( 0x1 ),	/* 1 */
/* 4108 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter app_id */

/* 4110 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 4112 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4114 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Parameter value */

/* 4116 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 4118 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4120 */	NdrFcShort( 0x44c ),	/* Type Offset=1100 */

	/* Return value */

/* 4122 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4124 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4126 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_isRollbackToTargetVersionAllowed */

/* 4128 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4130 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4134 */	NdrFcShort( 0x15 ),	/* 21 */
/* 4136 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 4138 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4140 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4142 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 4144 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 4146 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4148 */	NdrFcShort( 0x1 ),	/* 1 */
/* 4150 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter app_id */

/* 4152 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 4154 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4156 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Parameter value */

/* 4158 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 4160 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4162 */	NdrFcShort( 0x44c ),	/* Type Offset=1100 */

	/* Return value */

/* 4164 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4166 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4168 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_targetChannel */

/* 4170 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4172 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4176 */	NdrFcShort( 0x16 ),	/* 22 */
/* 4178 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 4180 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4182 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4184 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 4186 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 4188 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4190 */	NdrFcShort( 0x1 ),	/* 1 */
/* 4192 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter app_id */

/* 4194 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 4196 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4198 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Parameter value */

/* 4200 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 4202 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4204 */	NdrFcShort( 0x44c ),	/* Type Offset=1100 */

	/* Return value */

/* 4206 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4208 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4210 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_forceInstallApps */

/* 4212 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4214 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4218 */	NdrFcShort( 0x17 ),	/* 23 */
/* 4220 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 4222 */	NdrFcShort( 0x6 ),	/* 6 */
/* 4224 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4226 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x3,		/* 3 */
/* 4228 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 4230 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4232 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4234 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter is_machine */

/* 4236 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 4238 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4240 */	0x6,		/* FC_SHORT */
			0x0,		/* 0 */

	/* Parameter value */

/* 4242 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 4244 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4246 */	NdrFcShort( 0x44c ),	/* Type Offset=1100 */

	/* Return value */

/* 4248 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4250 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4252 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure setOriginURL */

/* 4254 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4256 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4260 */	NdrFcShort( 0x3 ),	/* 3 */
/* 4262 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4264 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4266 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4268 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 4270 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 4272 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4274 */	NdrFcShort( 0x1 ),	/* 1 */
/* 4276 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter origin_url */

/* 4278 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 4280 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4282 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Return value */

/* 4284 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4286 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4288 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure createApp */

/* 4290 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4292 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4296 */	NdrFcShort( 0x7 ),	/* 7 */
/* 4298 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 4300 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4302 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4304 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x5,		/* 5 */
/* 4306 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 4308 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4310 */	NdrFcShort( 0x1 ),	/* 1 */
/* 4312 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter app_guid */

/* 4314 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 4316 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4318 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Parameter brand_code */

/* 4320 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 4322 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4324 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Parameter language */

/* 4326 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 4328 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4330 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Parameter ap */

/* 4332 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 4334 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 4336 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Return value */

/* 4338 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4340 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 4342 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure put_parentHWND */

/* 4344 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4346 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4350 */	NdrFcShort( 0xc ),	/* 12 */
/* 4352 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4354 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4356 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4358 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 4360 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 4362 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4364 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4366 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter hwnd */

/* 4368 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 4370 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4372 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 4374 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4376 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4378 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_appWeb */

/* 4380 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4382 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4386 */	NdrFcShort( 0xe ),	/* 14 */
/* 4388 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 4390 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4392 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4394 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x3,		/* 3 */
/* 4396 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 4398 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4400 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4402 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter index */

/* 4404 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 4406 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4408 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter app_web */

/* 4410 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 4412 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4414 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Return value */

/* 4416 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4418 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4420 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure initialize */

/* 4422 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4424 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4428 */	NdrFcShort( 0xf ),	/* 15 */
/* 4430 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4432 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4434 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4436 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 4438 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 4440 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4442 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4444 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 4446 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4448 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4450 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure checkForUpdate */

/* 4452 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4454 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4458 */	NdrFcShort( 0x10 ),	/* 16 */
/* 4460 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4462 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4464 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4466 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 4468 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 4470 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4472 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4474 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 4476 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4478 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4480 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure download */

/* 4482 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4484 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4488 */	NdrFcShort( 0x11 ),	/* 17 */
/* 4490 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4492 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4494 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4496 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 4498 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 4500 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4502 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4504 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 4506 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4508 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4510 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure install */

/* 4512 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4514 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4518 */	NdrFcShort( 0x12 ),	/* 18 */
/* 4520 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4522 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4524 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4526 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 4528 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 4530 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4532 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4534 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 4536 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4538 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4540 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure pause */

/* 4542 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4544 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4548 */	NdrFcShort( 0x13 ),	/* 19 */
/* 4550 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4552 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4554 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4556 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 4558 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 4560 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4562 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4564 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 4566 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4568 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4570 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure resume */

/* 4572 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4574 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4578 */	NdrFcShort( 0x14 ),	/* 20 */
/* 4580 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4582 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4584 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4586 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 4588 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 4590 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4592 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4594 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 4596 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4598 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4600 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure cancel */

/* 4602 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4604 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4608 */	NdrFcShort( 0x15 ),	/* 21 */
/* 4610 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4612 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4614 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4616 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 4618 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 4620 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4622 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4624 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 4626 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4628 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4630 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure downloadPackage */

/* 4632 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4634 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4638 */	NdrFcShort( 0x16 ),	/* 22 */
/* 4640 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 4642 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4644 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4646 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 4648 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 4650 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4652 */	NdrFcShort( 0x1 ),	/* 1 */
/* 4654 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter app_id */

/* 4656 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 4658 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4660 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Parameter package_name */

/* 4662 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 4664 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4666 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Return value */

/* 4668 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4670 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4672 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_currentState */

/* 4674 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4676 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4680 */	NdrFcShort( 0x17 ),	/* 23 */
/* 4682 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4684 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4686 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4688 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 4690 */	0x8,		/* 8 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 4692 */	NdrFcShort( 0x1 ),	/* 1 */
/* 4694 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4696 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter current_state */

/* 4698 */	NdrFcShort( 0x4113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=16 */
/* 4700 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4702 */	NdrFcShort( 0x42c ),	/* Type Offset=1068 */

	/* Return value */

/* 4704 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4706 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4708 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_command */

/* 4710 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4712 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4716 */	NdrFcShort( 0xa ),	/* 10 */
/* 4718 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 4720 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4722 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4724 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 4726 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 4728 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4730 */	NdrFcShort( 0x1 ),	/* 1 */
/* 4732 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter command_id */

/* 4734 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 4736 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4738 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Parameter command */

/* 4740 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 4742 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4744 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Return value */

/* 4746 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4748 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4750 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure cancel */

/* 4752 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4754 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4758 */	NdrFcShort( 0xb ),	/* 11 */
/* 4760 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4762 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4764 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4766 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 4768 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 4770 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4772 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4774 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 4776 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4778 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4780 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure get_currentState */

/* 4782 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4784 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4788 */	NdrFcShort( 0xc ),	/* 12 */
/* 4790 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4792 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4794 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4796 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x2,		/* 2 */
/* 4798 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 4800 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4802 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4804 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter current_state */

/* 4806 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 4808 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4810 */	NdrFcShort( 0x6 ),	/* Type Offset=6 */

	/* Return value */

/* 4812 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4814 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4816 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure launch */

/* 4818 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4820 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4824 */	NdrFcShort( 0xd ),	/* 13 */
/* 4826 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4828 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4830 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4832 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 4834 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 4836 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4838 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4840 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 4842 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4844 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4846 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure uninstall */

/* 4848 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4850 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4854 */	NdrFcShort( 0xe ),	/* 14 */
/* 4856 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4858 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4860 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4862 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 4864 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 4866 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4868 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4870 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 4872 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4874 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4876 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure createOmahaMachineServerAsync */

/* 4878 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4880 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4884 */	NdrFcShort( 0x3 ),	/* 3 */
/* 4886 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 4888 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4890 */	NdrFcShort( 0x8 ),	/* 8 */
/* 4892 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 4894 */	0x8,		/* 8 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 4896 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4898 */	NdrFcShort( 0x1 ),	/* 1 */
/* 4900 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter origin_url */

/* 4902 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 4904 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4906 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Parameter create_elevated */

/* 4908 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 4910 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4912 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter status */

/* 4914 */	NdrFcShort( 0x13 ),	/* Flags:  must size, must free, out, */
/* 4916 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4918 */	NdrFcShort( 0x466 ),	/* Type Offset=1126 */

	/* Return value */

/* 4920 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4922 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 4924 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Send */

/* 4926 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 4928 */	NdrFcLong( 0x0 ),	/* 0 */
/* 4932 */	NdrFcShort( 0x3 ),	/* 3 */
/* 4934 */	NdrFcShort( 0x30 ),	/* x86 Stack size/offset = 48 */
/* 4936 */	NdrFcShort( 0x0 ),	/* 0 */
/* 4938 */	NdrFcShort( 0x24 ),	/* 36 */
/* 4940 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x8,		/* 8 */
/* 4942 */	0x8,		/* 8 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 4944 */	NdrFcShort( 0x1 ),	/* 1 */
/* 4946 */	NdrFcShort( 0x1 ),	/* 1 */
/* 4948 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter url */

/* 4950 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 4952 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 4954 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Parameter post_data */

/* 4956 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 4958 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 4960 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Parameter request_headers */

/* 4962 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 4964 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 4966 */	NdrFcShort( 0x48 ),	/* Type Offset=72 */

	/* Parameter response_headers_needed */

/* 4968 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 4970 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 4972 */	NdrFcShort( 0x43a ),	/* Type Offset=1082 */

	/* Parameter response_headers */

/* 4974 */	NdrFcShort( 0x4113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=16 */
/* 4976 */	NdrFcShort( 0x20 ),	/* x86 Stack size/offset = 32 */
/* 4978 */	NdrFcShort( 0x42c ),	/* Type Offset=1068 */

	/* Parameter response_code */

/* 4980 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 4982 */	NdrFcShort( 0x24 ),	/* x86 Stack size/offset = 36 */
/* 4984 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter cache_filename */

/* 4986 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 4988 */	NdrFcShort( 0x28 ),	/* x86 Stack size/offset = 40 */
/* 4990 */	NdrFcShort( 0x3a ),	/* Type Offset=58 */

	/* Return value */

/* 4992 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 4994 */	NdrFcShort( 0x2c ),	/* x86 Stack size/offset = 44 */
/* 4996 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LaunchCmdLine */

/* 4998 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5000 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5004 */	NdrFcShort( 0x3 ),	/* 3 */
/* 5006 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 5008 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5010 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5012 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 5014 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 5016 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5018 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5020 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter cmd_line */

/* 5022 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 5024 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5026 */	NdrFcShort( 0x47e ),	/* Type Offset=1150 */

	/* Return value */

/* 5028 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5030 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5032 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LaunchBrowser */

/* 5034 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5036 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5040 */	NdrFcShort( 0x4 ),	/* 4 */
/* 5042 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 5044 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5046 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5048 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 5050 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 5052 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5054 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5056 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter browser_type */

/* 5058 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 5060 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5062 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter url */

/* 5064 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 5066 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5068 */	NdrFcShort( 0x47e ),	/* Type Offset=1150 */

	/* Return value */

/* 5070 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5072 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 5074 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LaunchCmdElevated */

/* 5076 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5078 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5082 */	NdrFcShort( 0x5 ),	/* 5 */
/* 5084 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 5086 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5088 */	NdrFcShort( 0x24 ),	/* 36 */
/* 5090 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x5,		/* 5 */
/* 5092 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 5094 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5096 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5098 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter app_guid */

/* 5100 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 5102 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5104 */	NdrFcShort( 0x47e ),	/* Type Offset=1150 */

	/* Parameter cmd_id */

/* 5106 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 5108 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5110 */	NdrFcShort( 0x47e ),	/* Type Offset=1150 */

	/* Parameter caller_proc_id */

/* 5112 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 5114 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 5116 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter proc_handle */

/* 5118 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 5120 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 5122 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 5124 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5126 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 5128 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LaunchCmdLineEx */

/* 5130 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5132 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5136 */	NdrFcShort( 0x6 ),	/* 6 */
/* 5138 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 5140 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5142 */	NdrFcShort( 0x5c ),	/* 92 */
/* 5144 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x5,		/* 5 */
/* 5146 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 5148 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5150 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5152 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter cmd_line */

/* 5154 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 5156 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5158 */	NdrFcShort( 0x47e ),	/* Type Offset=1150 */

	/* Parameter server_proc_id */

/* 5160 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 5162 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5164 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter proc_handle */

/* 5166 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 5168 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 5170 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter stdout_handle */

/* 5172 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 5174 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 5176 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 5178 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5180 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 5182 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure OnShow */


	/* Procedure DoClose */

/* 5184 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5186 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5190 */	NdrFcShort( 0x3 ),	/* 3 */
/* 5192 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5194 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5196 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5198 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 5200 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 5202 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5204 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5206 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */


	/* Return value */

/* 5208 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5210 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5212 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure OnCheckingForUpdate */


	/* Procedure DoPause */

/* 5214 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5216 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5220 */	NdrFcShort( 0x4 ),	/* 4 */
/* 5222 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5224 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5226 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5228 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 5230 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 5232 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5234 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5236 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */


	/* Return value */

/* 5238 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5240 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5242 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure DoResume */

/* 5244 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5246 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5250 */	NdrFcShort( 0x5 ),	/* 5 */
/* 5252 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5254 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5256 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5258 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 5260 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 5262 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5264 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5266 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 5268 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5270 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5272 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure OnWaitingToDownload */


	/* Procedure DoRestartBrowsers */

/* 5274 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5276 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5280 */	NdrFcShort( 0x6 ),	/* 6 */
/* 5282 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5284 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5286 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5288 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 5290 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 5292 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5294 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5296 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */


	/* Return value */

/* 5298 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5300 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5302 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure DoReboot */

/* 5304 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5306 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5310 */	NdrFcShort( 0x7 ),	/* 7 */
/* 5312 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5314 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5316 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5318 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 5320 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 5322 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5324 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5326 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 5328 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5330 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5332 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure DoLaunchBrowser */

/* 5334 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5336 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5340 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5342 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 5344 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5346 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5348 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 5350 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 5352 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5354 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5356 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter url */

/* 5358 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 5360 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5362 */	NdrFcShort( 0x47e ),	/* Type Offset=1150 */

	/* Return value */

/* 5364 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5366 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5368 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure OnUpdateAvailable */

/* 5370 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5372 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5376 */	NdrFcShort( 0x5 ),	/* 5 */
/* 5378 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 5380 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5382 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5384 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 5386 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 5388 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5390 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5392 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter version_string */

/* 5394 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 5396 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5398 */	NdrFcShort( 0x47e ),	/* Type Offset=1150 */

	/* Return value */

/* 5400 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5402 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5404 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure OnDownloading */

/* 5406 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5408 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5412 */	NdrFcShort( 0x7 ),	/* 7 */
/* 5414 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 5416 */	NdrFcShort( 0x10 ),	/* 16 */
/* 5418 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5420 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x3,		/* 3 */
/* 5422 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 5424 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5426 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5428 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter time_remaining_ms */

/* 5430 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 5432 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5434 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pos */

/* 5436 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 5438 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5440 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 5442 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5444 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 5446 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure OnWaitingToInstall */

/* 5448 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5450 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5454 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5456 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5458 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5460 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5462 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 5464 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 5466 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5468 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5470 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 5472 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5474 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5476 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure OnPause */

/* 5478 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5480 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5484 */	NdrFcShort( 0xa ),	/* 10 */
/* 5486 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5488 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5490 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5492 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 5494 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 5496 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5498 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5500 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 5502 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5504 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5506 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure OnComplete */

/* 5508 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5510 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5514 */	NdrFcShort( 0xb ),	/* 11 */
/* 5516 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 5518 */	NdrFcShort( 0x6 ),	/* 6 */
/* 5520 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5522 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 5524 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 5526 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5528 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5530 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter code */

/* 5532 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 5534 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5536 */	0xd,		/* FC_ENUM16 */
			0x0,		/* 0 */

	/* Parameter completion_text */

/* 5538 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 5540 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5542 */	NdrFcShort( 0x47e ),	/* Type Offset=1150 */

	/* Return value */

/* 5544 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5546 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 5548 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure SetEventSink */

/* 5550 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5552 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5556 */	NdrFcShort( 0xc ),	/* 12 */
/* 5558 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 5560 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5562 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5564 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x2,		/* 2 */
/* 5566 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 5568 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5570 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5572 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ui_sink */

/* 5574 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 5576 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5578 */	NdrFcShort( 0x480 ),	/* Type Offset=1152 */

	/* Return value */

/* 5580 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5582 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5584 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure OnInstalling2 */

/* 5586 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5588 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5592 */	NdrFcShort( 0x3 ),	/* 3 */
/* 5594 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 5596 */	NdrFcShort( 0x10 ),	/* 16 */
/* 5598 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5600 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x3,		/* 3 */
/* 5602 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 5604 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5606 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5608 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter time_remaining_ms */

/* 5610 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 5612 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5614 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter pos */

/* 5616 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 5618 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5620 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 5622 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5624 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 5626 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure CheckForUpdate */

/* 5628 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5630 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5634 */	NdrFcShort( 0x3 ),	/* 3 */
/* 5636 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 5638 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5640 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5642 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 5644 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 5646 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5648 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5650 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter guid */

/* 5652 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 5654 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5656 */	NdrFcShort( 0x47e ),	/* Type Offset=1150 */

	/* Parameter observer */

/* 5658 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 5660 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5662 */	NdrFcShort( 0x492 ),	/* Type Offset=1170 */

	/* Return value */

/* 5664 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5666 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 5668 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure Update */

/* 5670 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5672 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5676 */	NdrFcShort( 0x4 ),	/* 4 */
/* 5678 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 5680 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5682 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5684 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x3,		/* 3 */
/* 5686 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 5688 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5690 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5692 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter guid */

/* 5694 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 5696 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5698 */	NdrFcShort( 0x47e ),	/* Type Offset=1150 */

	/* Parameter observer */

/* 5700 */	NdrFcShort( 0xb ),	/* Flags:  must size, must free, in, */
/* 5702 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5704 */	NdrFcShort( 0x492 ),	/* Type Offset=1170 */

	/* Return value */

/* 5706 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5708 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 5710 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure LaunchCmdElevated */

/* 5712 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 5714 */	NdrFcLong( 0x0 ),	/* 0 */
/* 5718 */	NdrFcShort( 0x3 ),	/* 3 */
/* 5720 */	NdrFcShort( 0x18 ),	/* x86 Stack size/offset = 24 */
/* 5722 */	NdrFcShort( 0x8 ),	/* 8 */
/* 5724 */	NdrFcShort( 0x24 ),	/* 36 */
/* 5726 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x5,		/* 5 */
/* 5728 */	0x8,		/* 8 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 5730 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5732 */	NdrFcShort( 0x0 ),	/* 0 */
/* 5734 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter app_guid */

/* 5736 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 5738 */	NdrFcShort( 0x4 ),	/* x86 Stack size/offset = 4 */
/* 5740 */	NdrFcShort( 0x47e ),	/* Type Offset=1150 */

	/* Parameter cmd_id */

/* 5742 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 5744 */	NdrFcShort( 0x8 ),	/* x86 Stack size/offset = 8 */
/* 5746 */	NdrFcShort( 0x47e ),	/* Type Offset=1150 */

	/* Parameter caller_proc_id */

/* 5748 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 5750 */	NdrFcShort( 0xc ),	/* x86 Stack size/offset = 12 */
/* 5752 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter proc_handle */

/* 5754 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 5756 */	NdrFcShort( 0x10 ),	/* x86 Stack size/offset = 16 */
/* 5758 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 5760 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 5762 */	NdrFcShort( 0x14 ),	/* x86 Stack size/offset = 20 */
/* 5764 */	0x8,		/* FC_LONG */
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
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
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
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
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
/* 62 */	NdrFcShort( 0x4 ),	/* 4 */
/* 64 */	NdrFcShort( 0x0 ),	/* 0 */
/* 66 */	NdrFcShort( 0xffde ),	/* Offset= -34 (32) */
/* 68 */	
			0x12, 0x0,	/* FC_UP */
/* 70 */	NdrFcShort( 0xffea ),	/* Offset= -22 (48) */
/* 72 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 74 */	NdrFcShort( 0x0 ),	/* 0 */
/* 76 */	NdrFcShort( 0x4 ),	/* 4 */
/* 78 */	NdrFcShort( 0x0 ),	/* 0 */
/* 80 */	NdrFcShort( 0xfff4 ),	/* Offset= -12 (68) */
/* 82 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 84 */	0x6,		/* FC_SHORT */
			0x5c,		/* FC_PAD */
/* 86 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 88 */	NdrFcShort( 0x3d4 ),	/* Offset= 980 (1068) */
/* 90 */	
			0x13, 0x0,	/* FC_OP */
/* 92 */	NdrFcShort( 0x3bc ),	/* Offset= 956 (1048) */
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
/* 196 */	NdrFcShort( 0x30a ),	/* Offset= 778 (974) */
/* 198 */	NdrFcLong( 0x4024 ),	/* 16420 */
/* 202 */	NdrFcShort( 0x304 ),	/* Offset= 772 (974) */
/* 204 */	NdrFcLong( 0x4011 ),	/* 16401 */
/* 208 */	NdrFcShort( 0x302 ),	/* Offset= 770 (978) */
/* 210 */	NdrFcLong( 0x4002 ),	/* 16386 */
/* 214 */	NdrFcShort( 0x300 ),	/* Offset= 768 (982) */
/* 216 */	NdrFcLong( 0x4003 ),	/* 16387 */
/* 220 */	NdrFcShort( 0x2fe ),	/* Offset= 766 (986) */
/* 222 */	NdrFcLong( 0x4014 ),	/* 16404 */
/* 226 */	NdrFcShort( 0x2fc ),	/* Offset= 764 (990) */
/* 228 */	NdrFcLong( 0x4004 ),	/* 16388 */
/* 232 */	NdrFcShort( 0x2fa ),	/* Offset= 762 (994) */
/* 234 */	NdrFcLong( 0x4005 ),	/* 16389 */
/* 238 */	NdrFcShort( 0x2f8 ),	/* Offset= 760 (998) */
/* 240 */	NdrFcLong( 0x400b ),	/* 16395 */
/* 244 */	NdrFcShort( 0x2e2 ),	/* Offset= 738 (982) */
/* 246 */	NdrFcLong( 0x400a ),	/* 16394 */
/* 250 */	NdrFcShort( 0x2e0 ),	/* Offset= 736 (986) */
/* 252 */	NdrFcLong( 0x4006 ),	/* 16390 */
/* 256 */	NdrFcShort( 0x2ea ),	/* Offset= 746 (1002) */
/* 258 */	NdrFcLong( 0x4007 ),	/* 16391 */
/* 262 */	NdrFcShort( 0x2e0 ),	/* Offset= 736 (998) */
/* 264 */	NdrFcLong( 0x4008 ),	/* 16392 */
/* 268 */	NdrFcShort( 0x2e2 ),	/* Offset= 738 (1006) */
/* 270 */	NdrFcLong( 0x400d ),	/* 16397 */
/* 274 */	NdrFcShort( 0x2e0 ),	/* Offset= 736 (1010) */
/* 276 */	NdrFcLong( 0x4009 ),	/* 16393 */
/* 280 */	NdrFcShort( 0x2de ),	/* Offset= 734 (1014) */
/* 282 */	NdrFcLong( 0x6000 ),	/* 24576 */
/* 286 */	NdrFcShort( 0x2dc ),	/* Offset= 732 (1018) */
/* 288 */	NdrFcLong( 0x400c ),	/* 16396 */
/* 292 */	NdrFcShort( 0x2da ),	/* Offset= 730 (1022) */
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
/* 334 */	NdrFcShort( 0x2b8 ),	/* Offset= 696 (1030) */
/* 336 */	NdrFcLong( 0x400e ),	/* 16398 */
/* 340 */	NdrFcShort( 0x2bc ),	/* Offset= 700 (1040) */
/* 342 */	NdrFcLong( 0x4010 ),	/* 16400 */
/* 346 */	NdrFcShort( 0x2ba ),	/* Offset= 698 (1044) */
/* 348 */	NdrFcLong( 0x4012 ),	/* 16402 */
/* 352 */	NdrFcShort( 0x276 ),	/* Offset= 630 (982) */
/* 354 */	NdrFcLong( 0x4013 ),	/* 16403 */
/* 358 */	NdrFcShort( 0x274 ),	/* Offset= 628 (986) */
/* 360 */	NdrFcLong( 0x4015 ),	/* 16405 */
/* 364 */	NdrFcShort( 0x272 ),	/* Offset= 626 (990) */
/* 366 */	NdrFcLong( 0x4016 ),	/* 16406 */
/* 370 */	NdrFcShort( 0x268 ),	/* Offset= 616 (986) */
/* 372 */	NdrFcLong( 0x4017 ),	/* 16407 */
/* 376 */	NdrFcShort( 0x262 ),	/* Offset= 610 (986) */
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
/* 422 */	NdrFcShort( 0x216 ),	/* Offset= 534 (956) */
/* 424 */	
			0x2a,		/* FC_ENCAPSULATED_UNION */
			0x49,		/* 73 */
/* 426 */	NdrFcShort( 0x18 ),	/* 24 */
/* 428 */	NdrFcShort( 0xa ),	/* 10 */
/* 430 */	NdrFcLong( 0x8 ),	/* 8 */
/* 434 */	NdrFcShort( 0x5a ),	/* Offset= 90 (524) */
/* 436 */	NdrFcLong( 0xd ),	/* 13 */
/* 440 */	NdrFcShort( 0x7e ),	/* Offset= 126 (566) */
/* 442 */	NdrFcLong( 0x9 ),	/* 9 */
/* 446 */	NdrFcShort( 0x9e ),	/* Offset= 158 (604) */
/* 448 */	NdrFcLong( 0xc ),	/* 12 */
/* 452 */	NdrFcShort( 0xc8 ),	/* Offset= 200 (652) */
/* 454 */	NdrFcLong( 0x24 ),	/* 36 */
/* 458 */	NdrFcShort( 0x124 ),	/* Offset= 292 (750) */
/* 460 */	NdrFcLong( 0x800d ),	/* 32781 */
/* 464 */	NdrFcShort( 0x140 ),	/* Offset= 320 (784) */
/* 466 */	NdrFcLong( 0x10 ),	/* 16 */
/* 470 */	NdrFcShort( 0x15a ),	/* Offset= 346 (816) */
/* 472 */	NdrFcLong( 0x2 ),	/* 2 */
/* 476 */	NdrFcShort( 0x174 ),	/* Offset= 372 (848) */
/* 478 */	NdrFcLong( 0x3 ),	/* 3 */
/* 482 */	NdrFcShort( 0x18e ),	/* Offset= 398 (880) */
/* 484 */	NdrFcLong( 0x14 ),	/* 20 */
/* 488 */	NdrFcShort( 0x1a8 ),	/* Offset= 424 (912) */
/* 490 */	NdrFcShort( 0xffff ),	/* Offset= -1 (489) */
/* 492 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 494 */	NdrFcShort( 0x4 ),	/* 4 */
/* 496 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 498 */	NdrFcShort( 0x0 ),	/* 0 */
/* 500 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 502 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 504 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 506 */	NdrFcShort( 0x4 ),	/* 4 */
/* 508 */	NdrFcShort( 0x0 ),	/* 0 */
/* 510 */	NdrFcShort( 0x1 ),	/* 1 */
/* 512 */	NdrFcShort( 0x0 ),	/* 0 */
/* 514 */	NdrFcShort( 0x0 ),	/* 0 */
/* 516 */	0x13, 0x0,	/* FC_OP */
/* 518 */	NdrFcShort( 0xfe2a ),	/* Offset= -470 (48) */
/* 520 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 522 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 524 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 526 */	NdrFcShort( 0x8 ),	/* 8 */
/* 528 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 530 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 532 */	NdrFcShort( 0x4 ),	/* 4 */
/* 534 */	NdrFcShort( 0x4 ),	/* 4 */
/* 536 */	0x11, 0x0,	/* FC_RP */
/* 538 */	NdrFcShort( 0xffd2 ),	/* Offset= -46 (492) */
/* 540 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 542 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 544 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 546 */	NdrFcShort( 0x0 ),	/* 0 */
/* 548 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 550 */	NdrFcShort( 0x0 ),	/* 0 */
/* 552 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 554 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 558 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 560 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 562 */	NdrFcShort( 0xff5c ),	/* Offset= -164 (398) */
/* 564 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 566 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 568 */	NdrFcShort( 0x8 ),	/* 8 */
/* 570 */	NdrFcShort( 0x0 ),	/* 0 */
/* 572 */	NdrFcShort( 0x6 ),	/* Offset= 6 (578) */
/* 574 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 576 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 578 */	
			0x11, 0x0,	/* FC_RP */
/* 580 */	NdrFcShort( 0xffdc ),	/* Offset= -36 (544) */
/* 582 */	
			0x21,		/* FC_BOGUS_ARRAY */
			0x3,		/* 3 */
/* 584 */	NdrFcShort( 0x0 ),	/* 0 */
/* 586 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 588 */	NdrFcShort( 0x0 ),	/* 0 */
/* 590 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 592 */	NdrFcLong( 0xffffffff ),	/* -1 */
/* 596 */	NdrFcShort( 0x0 ),	/* Corr flags:  */
/* 598 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 600 */	NdrFcShort( 0xfdb2 ),	/* Offset= -590 (10) */
/* 602 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 604 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 606 */	NdrFcShort( 0x8 ),	/* 8 */
/* 608 */	NdrFcShort( 0x0 ),	/* 0 */
/* 610 */	NdrFcShort( 0x6 ),	/* Offset= 6 (616) */
/* 612 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 614 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 616 */	
			0x11, 0x0,	/* FC_RP */
/* 618 */	NdrFcShort( 0xffdc ),	/* Offset= -36 (582) */
/* 620 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 622 */	NdrFcShort( 0x4 ),	/* 4 */
/* 624 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 626 */	NdrFcShort( 0x0 ),	/* 0 */
/* 628 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 630 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 632 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 634 */	NdrFcShort( 0x4 ),	/* 4 */
/* 636 */	NdrFcShort( 0x0 ),	/* 0 */
/* 638 */	NdrFcShort( 0x1 ),	/* 1 */
/* 640 */	NdrFcShort( 0x0 ),	/* 0 */
/* 642 */	NdrFcShort( 0x0 ),	/* 0 */
/* 644 */	0x13, 0x0,	/* FC_OP */
/* 646 */	NdrFcShort( 0x192 ),	/* Offset= 402 (1048) */
/* 648 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 650 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 652 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 654 */	NdrFcShort( 0x8 ),	/* 8 */
/* 656 */	NdrFcShort( 0x0 ),	/* 0 */
/* 658 */	NdrFcShort( 0x6 ),	/* Offset= 6 (664) */
/* 660 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 662 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 664 */	
			0x11, 0x0,	/* FC_RP */
/* 666 */	NdrFcShort( 0xffd2 ),	/* Offset= -46 (620) */
/* 668 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 670 */	NdrFcLong( 0x2f ),	/* 47 */
/* 674 */	NdrFcShort( 0x0 ),	/* 0 */
/* 676 */	NdrFcShort( 0x0 ),	/* 0 */
/* 678 */	0xc0,		/* 192 */
			0x0,		/* 0 */
/* 680 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 682 */	0x0,		/* 0 */
			0x0,		/* 0 */
/* 684 */	0x0,		/* 0 */
			0x46,		/* 70 */
/* 686 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 688 */	NdrFcShort( 0x1 ),	/* 1 */
/* 690 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 692 */	NdrFcShort( 0x4 ),	/* 4 */
/* 694 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 696 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 698 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 700 */	NdrFcShort( 0x10 ),	/* 16 */
/* 702 */	NdrFcShort( 0x0 ),	/* 0 */
/* 704 */	NdrFcShort( 0xa ),	/* Offset= 10 (714) */
/* 706 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 708 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 710 */	NdrFcShort( 0xffd6 ),	/* Offset= -42 (668) */
/* 712 */	0x36,		/* FC_POINTER */
			0x5b,		/* FC_END */
/* 714 */	
			0x13, 0x0,	/* FC_OP */
/* 716 */	NdrFcShort( 0xffe2 ),	/* Offset= -30 (686) */
/* 718 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 720 */	NdrFcShort( 0x4 ),	/* 4 */
/* 722 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 724 */	NdrFcShort( 0x0 ),	/* 0 */
/* 726 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 728 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 730 */	
			0x48,		/* FC_VARIABLE_REPEAT */
			0x49,		/* FC_FIXED_OFFSET */
/* 732 */	NdrFcShort( 0x4 ),	/* 4 */
/* 734 */	NdrFcShort( 0x0 ),	/* 0 */
/* 736 */	NdrFcShort( 0x1 ),	/* 1 */
/* 738 */	NdrFcShort( 0x0 ),	/* 0 */
/* 740 */	NdrFcShort( 0x0 ),	/* 0 */
/* 742 */	0x13, 0x0,	/* FC_OP */
/* 744 */	NdrFcShort( 0xffd2 ),	/* Offset= -46 (698) */
/* 746 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 748 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 750 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 752 */	NdrFcShort( 0x8 ),	/* 8 */
/* 754 */	NdrFcShort( 0x0 ),	/* 0 */
/* 756 */	NdrFcShort( 0x6 ),	/* Offset= 6 (762) */
/* 758 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 760 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 762 */	
			0x11, 0x0,	/* FC_RP */
/* 764 */	NdrFcShort( 0xffd2 ),	/* Offset= -46 (718) */
/* 766 */	
			0x1d,		/* FC_SMFARRAY */
			0x0,		/* 0 */
/* 768 */	NdrFcShort( 0x8 ),	/* 8 */
/* 770 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 772 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 774 */	NdrFcShort( 0x10 ),	/* 16 */
/* 776 */	0x8,		/* FC_LONG */
			0x6,		/* FC_SHORT */
/* 778 */	0x6,		/* FC_SHORT */
			0x4c,		/* FC_EMBEDDED_COMPLEX */
/* 780 */	0x0,		/* 0 */
			NdrFcShort( 0xfff1 ),	/* Offset= -15 (766) */
			0x5b,		/* FC_END */
/* 784 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 786 */	NdrFcShort( 0x18 ),	/* 24 */
/* 788 */	NdrFcShort( 0x0 ),	/* 0 */
/* 790 */	NdrFcShort( 0xa ),	/* Offset= 10 (800) */
/* 792 */	0x8,		/* FC_LONG */
			0x36,		/* FC_POINTER */
/* 794 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 796 */	NdrFcShort( 0xffe8 ),	/* Offset= -24 (772) */
/* 798 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 800 */	
			0x11, 0x0,	/* FC_RP */
/* 802 */	NdrFcShort( 0xfefe ),	/* Offset= -258 (544) */
/* 804 */	
			0x1b,		/* FC_CARRAY */
			0x0,		/* 0 */
/* 806 */	NdrFcShort( 0x1 ),	/* 1 */
/* 808 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 810 */	NdrFcShort( 0x0 ),	/* 0 */
/* 812 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 814 */	0x1,		/* FC_BYTE */
			0x5b,		/* FC_END */
/* 816 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 818 */	NdrFcShort( 0x8 ),	/* 8 */
/* 820 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 822 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 824 */	NdrFcShort( 0x4 ),	/* 4 */
/* 826 */	NdrFcShort( 0x4 ),	/* 4 */
/* 828 */	0x13, 0x0,	/* FC_OP */
/* 830 */	NdrFcShort( 0xffe6 ),	/* Offset= -26 (804) */
/* 832 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 834 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 836 */	
			0x1b,		/* FC_CARRAY */
			0x1,		/* 1 */
/* 838 */	NdrFcShort( 0x2 ),	/* 2 */
/* 840 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 842 */	NdrFcShort( 0x0 ),	/* 0 */
/* 844 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 846 */	0x6,		/* FC_SHORT */
			0x5b,		/* FC_END */
/* 848 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 850 */	NdrFcShort( 0x8 ),	/* 8 */
/* 852 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 854 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 856 */	NdrFcShort( 0x4 ),	/* 4 */
/* 858 */	NdrFcShort( 0x4 ),	/* 4 */
/* 860 */	0x13, 0x0,	/* FC_OP */
/* 862 */	NdrFcShort( 0xffe6 ),	/* Offset= -26 (836) */
/* 864 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 866 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 868 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 870 */	NdrFcShort( 0x4 ),	/* 4 */
/* 872 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 874 */	NdrFcShort( 0x0 ),	/* 0 */
/* 876 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 878 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 880 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 882 */	NdrFcShort( 0x8 ),	/* 8 */
/* 884 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 886 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 888 */	NdrFcShort( 0x4 ),	/* 4 */
/* 890 */	NdrFcShort( 0x4 ),	/* 4 */
/* 892 */	0x13, 0x0,	/* FC_OP */
/* 894 */	NdrFcShort( 0xffe6 ),	/* Offset= -26 (868) */
/* 896 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 898 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 900 */	
			0x1b,		/* FC_CARRAY */
			0x7,		/* 7 */
/* 902 */	NdrFcShort( 0x8 ),	/* 8 */
/* 904 */	0x19,		/* Corr desc:  field pointer, FC_ULONG */
			0x0,		/*  */
/* 906 */	NdrFcShort( 0x0 ),	/* 0 */
/* 908 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 910 */	0xb,		/* FC_HYPER */
			0x5b,		/* FC_END */
/* 912 */	
			0x16,		/* FC_PSTRUCT */
			0x3,		/* 3 */
/* 914 */	NdrFcShort( 0x8 ),	/* 8 */
/* 916 */	
			0x4b,		/* FC_PP */
			0x5c,		/* FC_PAD */
/* 918 */	
			0x46,		/* FC_NO_REPEAT */
			0x5c,		/* FC_PAD */
/* 920 */	NdrFcShort( 0x4 ),	/* 4 */
/* 922 */	NdrFcShort( 0x4 ),	/* 4 */
/* 924 */	0x13, 0x0,	/* FC_OP */
/* 926 */	NdrFcShort( 0xffe6 ),	/* Offset= -26 (900) */
/* 928 */	
			0x5b,		/* FC_END */

			0x8,		/* FC_LONG */
/* 930 */	0x8,		/* FC_LONG */
			0x5b,		/* FC_END */
/* 932 */	
			0x15,		/* FC_STRUCT */
			0x3,		/* 3 */
/* 934 */	NdrFcShort( 0x8 ),	/* 8 */
/* 936 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 938 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 940 */	
			0x1b,		/* FC_CARRAY */
			0x3,		/* 3 */
/* 942 */	NdrFcShort( 0x8 ),	/* 8 */
/* 944 */	0x7,		/* Corr desc: FC_USHORT */
			0x0,		/*  */
/* 946 */	NdrFcShort( 0xffd8 ),	/* -40 */
/* 948 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 950 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 952 */	NdrFcShort( 0xffec ),	/* Offset= -20 (932) */
/* 954 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 956 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x3,		/* 3 */
/* 958 */	NdrFcShort( 0x28 ),	/* 40 */
/* 960 */	NdrFcShort( 0xffec ),	/* Offset= -20 (940) */
/* 962 */	NdrFcShort( 0x0 ),	/* Offset= 0 (962) */
/* 964 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 966 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 968 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 970 */	NdrFcShort( 0xfdde ),	/* Offset= -546 (424) */
/* 972 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 974 */	
			0x13, 0x0,	/* FC_OP */
/* 976 */	NdrFcShort( 0xfeea ),	/* Offset= -278 (698) */
/* 978 */	
			0x13, 0x8,	/* FC_OP [simple_pointer] */
/* 980 */	0x1,		/* FC_BYTE */
			0x5c,		/* FC_PAD */
/* 982 */	
			0x13, 0x8,	/* FC_OP [simple_pointer] */
/* 984 */	0x6,		/* FC_SHORT */
			0x5c,		/* FC_PAD */
/* 986 */	
			0x13, 0x8,	/* FC_OP [simple_pointer] */
/* 988 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 990 */	
			0x13, 0x8,	/* FC_OP [simple_pointer] */
/* 992 */	0xb,		/* FC_HYPER */
			0x5c,		/* FC_PAD */
/* 994 */	
			0x13, 0x8,	/* FC_OP [simple_pointer] */
/* 996 */	0xa,		/* FC_FLOAT */
			0x5c,		/* FC_PAD */
/* 998 */	
			0x13, 0x8,	/* FC_OP [simple_pointer] */
/* 1000 */	0xc,		/* FC_DOUBLE */
			0x5c,		/* FC_PAD */
/* 1002 */	
			0x13, 0x0,	/* FC_OP */
/* 1004 */	NdrFcShort( 0xfd9c ),	/* Offset= -612 (392) */
/* 1006 */	
			0x13, 0x10,	/* FC_OP [pointer_deref] */
/* 1008 */	NdrFcShort( 0xfc30 ),	/* Offset= -976 (32) */
/* 1010 */	
			0x13, 0x10,	/* FC_OP [pointer_deref] */
/* 1012 */	NdrFcShort( 0xfd9a ),	/* Offset= -614 (398) */
/* 1014 */	
			0x13, 0x10,	/* FC_OP [pointer_deref] */
/* 1016 */	NdrFcShort( 0xfc12 ),	/* Offset= -1006 (10) */
/* 1018 */	
			0x13, 0x10,	/* FC_OP [pointer_deref] */
/* 1020 */	NdrFcShort( 0xfda4 ),	/* Offset= -604 (416) */
/* 1022 */	
			0x13, 0x10,	/* FC_OP [pointer_deref] */
/* 1024 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1026) */
/* 1026 */	
			0x13, 0x0,	/* FC_OP */
/* 1028 */	NdrFcShort( 0x14 ),	/* Offset= 20 (1048) */
/* 1030 */	
			0x15,		/* FC_STRUCT */
			0x7,		/* 7 */
/* 1032 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1034 */	0x6,		/* FC_SHORT */
			0x1,		/* FC_BYTE */
/* 1036 */	0x1,		/* FC_BYTE */
			0x8,		/* FC_LONG */
/* 1038 */	0xb,		/* FC_HYPER */
			0x5b,		/* FC_END */
/* 1040 */	
			0x13, 0x0,	/* FC_OP */
/* 1042 */	NdrFcShort( 0xfff4 ),	/* Offset= -12 (1030) */
/* 1044 */	
			0x13, 0x8,	/* FC_OP [simple_pointer] */
/* 1046 */	0x2,		/* FC_CHAR */
			0x5c,		/* FC_PAD */
/* 1048 */	
			0x1a,		/* FC_BOGUS_STRUCT */
			0x7,		/* 7 */
/* 1050 */	NdrFcShort( 0x20 ),	/* 32 */
/* 1052 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1054 */	NdrFcShort( 0x0 ),	/* Offset= 0 (1054) */
/* 1056 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 1058 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 1060 */	0x6,		/* FC_SHORT */
			0x6,		/* FC_SHORT */
/* 1062 */	0x4c,		/* FC_EMBEDDED_COMPLEX */
			0x0,		/* 0 */
/* 1064 */	NdrFcShort( 0xfc36 ),	/* Offset= -970 (94) */
/* 1066 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 1068 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 1070 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1072 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1074 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1076 */	NdrFcShort( 0xfc26 ),	/* Offset= -986 (90) */
/* 1078 */	
			0x12, 0x0,	/* FC_UP */
/* 1080 */	NdrFcShort( 0xffe0 ),	/* Offset= -32 (1048) */
/* 1082 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 1084 */	NdrFcShort( 0x1 ),	/* 1 */
/* 1086 */	NdrFcShort( 0x10 ),	/* 16 */
/* 1088 */	NdrFcShort( 0x0 ),	/* 0 */
/* 1090 */	NdrFcShort( 0xfff4 ),	/* Offset= -12 (1078) */
/* 1092 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 1094 */	0xb,		/* FC_HYPER */
			0x5c,		/* FC_PAD */
/* 1096 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 1098 */	0xc,		/* FC_DOUBLE */
			0x5c,		/* FC_PAD */
/* 1100 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 1102 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1104) */
/* 1104 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1106 */	NdrFcLong( 0x931e73fd ),	/* -1826720771 */
/* 1110 */	NdrFcShort( 0xd487 ),	/* -11129 */
/* 1112 */	NdrFcShort( 0x4458 ),	/* 17496 */
/* 1114 */	0xaa,		/* 170 */
			0x8,		/* 8 */
/* 1116 */	0x1f,		/* 31 */
			0xf4,		/* 244 */
/* 1118 */	0x14,		/* 20 */
			0x13,		/* 19 */
/* 1120 */	0x37,		/* 55 */
			0x7b,		/* 123 */
/* 1122 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 1124 */	0x6,		/* FC_SHORT */
			0x5c,		/* FC_PAD */
/* 1126 */	
			0x11, 0x10,	/* FC_RP [pointer_deref] */
/* 1128 */	NdrFcShort( 0x2 ),	/* Offset= 2 (1130) */
/* 1130 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1132 */	NdrFcLong( 0x7a24060e ),	/* 2049181198 */
/* 1136 */	NdrFcShort( 0x533f ),	/* 21311 */
/* 1138 */	NdrFcShort( 0x4962 ),	/* 18786 */
/* 1140 */	0x9e,		/* 158 */
			0x15,		/* 21 */
/* 1142 */	0x34,		/* 52 */
			0xbd,		/* 189 */
/* 1144 */	0x82,		/* 130 */
			0x55,		/* 85 */
/* 1146 */	0x5f,		/* 95 */
			0xa7,		/* 167 */
/* 1148 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/* 1150 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/* 1152 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1154 */	NdrFcLong( 0x8504fb26 ),	/* -2063271130 */
/* 1158 */	NdrFcShort( 0xfc3e ),	/* -962 */
/* 1160 */	NdrFcShort( 0x4c1c ),	/* 19484 */
/* 1162 */	0x9c,		/* 156 */
			0x94,		/* 148 */
/* 1164 */	0x46,		/* 70 */
			0xec,		/* 236 */
/* 1166 */	0x93,		/* 147 */
			0xe6,		/* 230 */
/* 1168 */	0xba,		/* 186 */
			0x63,		/* 99 */
/* 1170 */	
			0x2f,		/* FC_IP */
			0x5a,		/* FC_CONSTANT_IID */
/* 1172 */	NdrFcLong( 0x7cfc4e00 ),	/* 2096909824 */
/* 1176 */	NdrFcShort( 0x1c9d ),	/* 7325 */
/* 1178 */	NdrFcShort( 0x443d ),	/* 17469 */
/* 1180 */	0xb5,		/* 181 */
			0xbe,		/* 190 */
/* 1182 */	0xce,		/* 206 */
			0xee,		/* 238 */
/* 1184 */	0xac,		/* 172 */
			0x14,		/* 20 */
/* 1186 */	0x43,		/* 67 */
			0xaf,		/* 175 */

			0x0
        }
    };

XFG_TRAMPOLINES(BSTR)
XFG_TRAMPOLINES(VARIANT)

static const USER_MARSHAL_ROUTINE_QUADRUPLE UserMarshalRoutines[ WIRE_MARSHAL_TABLE_SIZE ] = 
        {
            
            {
            XFG_TRAMPOLINE_FPTR(BSTR_UserSize)
            ,XFG_TRAMPOLINE_FPTR(BSTR_UserMarshal)
            ,XFG_TRAMPOLINE_FPTR(BSTR_UserUnmarshal)
            ,XFG_TRAMPOLINE_FPTR(BSTR_UserFree)
            
            }
            ,
            {
            XFG_TRAMPOLINE_FPTR(VARIANT_UserSize)
            ,XFG_TRAMPOLINE_FPTR(VARIANT_UserMarshal)
            ,XFG_TRAMPOLINE_FPTR(VARIANT_UserUnmarshal)
            ,XFG_TRAMPOLINE_FPTR(VARIANT_UserFree)
            
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
    36,
    78
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
    114,
    150,
    186,
    222,
    258,
    294,
    330,
    366,
    402,
    438,
    474,
    510,
    546,
    582,
    618,
    654,
    690,
    726,
    768,
    816,
    852,
    882,
    924,
    966,
    996,
    1026,
    1056,
    1086,
    1116,
    1146,
    1176,
    1206,
    1242,
    1284
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
    1320,
    1356,
    186,
    1392,
    1428,
    1464,
    1500,
    1536,
    1572,
    1608,
    1644,
    1680,
    1716,
    1752,
    1788,
    1824,
    1860,
    1896,
    1932,
    1968,
    2004,
    882,
    2040,
    2076,
    2112,
    2148,
    2184,
    2220,
    2256,
    2292,
    2328,
    2364,
    2400,
    2436
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
    1320,
    1356,
    186,
    1392,
    1428,
    1464,
    1500,
    1536,
    1572,
    1608,
    1644,
    1680,
    1716,
    1752,
    1788,
    1824,
    1860,
    1896,
    1932,
    1968,
    2004,
    882,
    2040,
    2076,
    2112,
    2148,
    2184,
    2220,
    2256,
    2292,
    2328,
    2364,
    2400,
    2436,
    2472,
    2508
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
    2544,
    2580,
    2616,
    2652
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
    2544,
    2580,
    2616,
    2652,
    258
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
    114,
    2580,
    2736
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
    2778,
    2814,
    186
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
    2850,
    2616,
    2886,
    2922,
    2958,
    2994,
    3030,
    3066,
    3102,
    3138,
    1680,
    3174,
    3210,
    3246,
    1824,
    690
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
    3282
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
    3324
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


/* Object interface: IPolicyStatus, ver. 0.0,
   GUID={0x10DB7BD5,0xBD0B,0x4886,{0x97,0x05,0x17,0x42,0x03,0xFE,0x0A,0xDA}} */

#pragma code_seg(".orpc")
static const unsigned short IPolicyStatus_FormatStringOffsetTable[] =
    {
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    0,
    3384,
    186,
    2886,
    2922,
    3438,
    3480,
    3522,
    3564
    };

static const MIDL_STUBLESS_PROXY_INFO IPolicyStatus_ProxyInfo =
    {
    &Object_StubDesc,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IPolicyStatus_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IPolicyStatus_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IPolicyStatus_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(16) _IPolicyStatusProxyVtbl = 
{
    &IPolicyStatus_ProxyInfo,
    &IID_IPolicyStatus,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    0 /* IDispatch::GetTypeInfoCount */ ,
    0 /* IDispatch::GetTypeInfo */ ,
    0 /* IDispatch::GetIDsOfNames */ ,
    0 /* IDispatch_Invoke_Proxy */ ,
    (void *) (INT_PTR) -1 /* IPolicyStatus::get_lastCheckPeriodMinutes */ ,
    (void *) (INT_PTR) -1 /* IPolicyStatus::get_updatesSuppressedTimes */ ,
    (void *) (INT_PTR) -1 /* IPolicyStatus::get_downloadPreferenceGroupPolicy */ ,
    (void *) (INT_PTR) -1 /* IPolicyStatus::get_packageCacheSizeLimitMBytes */ ,
    (void *) (INT_PTR) -1 /* IPolicyStatus::get_packageCacheExpirationTimeDays */ ,
    (void *) (INT_PTR) -1 /* IPolicyStatus::get_effectivePolicyForAppInstalls */ ,
    (void *) (INT_PTR) -1 /* IPolicyStatus::get_effectivePolicyForAppUpdates */ ,
    (void *) (INT_PTR) -1 /* IPolicyStatus::get_targetVersionPrefix */ ,
    (void *) (INT_PTR) -1 /* IPolicyStatus::get_isRollbackToTargetVersionAllowed */
};


static const PRPC_STUB_FUNCTION IPolicyStatus_table[] =
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
    NdrStubCall2
};

CInterfaceStubVtbl _IPolicyStatusStubVtbl =
{
    &IID_IPolicyStatus,
    &IPolicyStatus_ServerInfo,
    16,
    &IPolicyStatus_table[-3],
    CStdStubBuffer_DELEGATING_METHODS
};


/* Object interface: IPolicyStatusValue, ver. 0.0,
   GUID={0x931E73FD,0xD487,0x4458,{0xAA,0x08,0x1F,0xF4,0x14,0x13,0x37,0x7B}} */

#pragma code_seg(".orpc")
static const unsigned short IPolicyStatusValue_FormatStringOffsetTable[] =
    {
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    114,
    2850,
    3606,
    1392,
    258
    };

static const MIDL_STUBLESS_PROXY_INFO IPolicyStatusValue_ProxyInfo =
    {
    &Object_StubDesc,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IPolicyStatusValue_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IPolicyStatusValue_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IPolicyStatusValue_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(12) _IPolicyStatusValueProxyVtbl = 
{
    &IPolicyStatusValue_ProxyInfo,
    &IID_IPolicyStatusValue,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    0 /* IDispatch::GetTypeInfoCount */ ,
    0 /* IDispatch::GetTypeInfo */ ,
    0 /* IDispatch::GetIDsOfNames */ ,
    0 /* IDispatch_Invoke_Proxy */ ,
    (void *) (INT_PTR) -1 /* IPolicyStatusValue::get_source */ ,
    (void *) (INT_PTR) -1 /* IPolicyStatusValue::get_value */ ,
    (void *) (INT_PTR) -1 /* IPolicyStatusValue::get_hasConflict */ ,
    (void *) (INT_PTR) -1 /* IPolicyStatusValue::get_conflictSource */ ,
    (void *) (INT_PTR) -1 /* IPolicyStatusValue::get_conflictValue */
};


static const PRPC_STUB_FUNCTION IPolicyStatusValue_table[] =
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

CInterfaceStubVtbl _IPolicyStatusValueStubVtbl =
{
    &IID_IPolicyStatusValue,
    &IPolicyStatusValue_ServerInfo,
    12,
    &IPolicyStatusValue_table[-3],
    CStdStubBuffer_DELEGATING_METHODS
};


/* Object interface: IPolicyStatus2, ver. 0.0,
   GUID={0xEFF9CA12,0x4CD3,0x474B,{0xB8,0x81,0xCD,0xE1,0xD9,0x2F,0x19,0x96}} */

#pragma code_seg(".orpc")
static const unsigned short IPolicyStatus2_FormatStringOffsetTable[] =
    {
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    114,
    3642,
    3678,
    3708,
    3744,
    3786,
    3822,
    3858,
    3894,
    3930,
    3966,
    4002,
    4044,
    4086,
    4128,
    4170
    };

static const MIDL_STUBLESS_PROXY_INFO IPolicyStatus2_ProxyInfo =
    {
    &Object_StubDesc,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IPolicyStatus2_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IPolicyStatus2_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IPolicyStatus2_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(23) _IPolicyStatus2ProxyVtbl = 
{
    &IPolicyStatus2_ProxyInfo,
    &IID_IPolicyStatus2,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    0 /* IDispatch::GetTypeInfoCount */ ,
    0 /* IDispatch::GetTypeInfo */ ,
    0 /* IDispatch::GetIDsOfNames */ ,
    0 /* IDispatch_Invoke_Proxy */ ,
    (void *) (INT_PTR) -1 /* IPolicyStatus2::get_updaterVersion */ ,
    (void *) (INT_PTR) -1 /* IPolicyStatus2::get_lastCheckedTime */ ,
    (void *) (INT_PTR) -1 /* IPolicyStatus2::refreshPolicies */ ,
    (void *) (INT_PTR) -1 /* IPolicyStatus2::get_lastCheckPeriodMinutes */ ,
    (void *) (INT_PTR) -1 /* IPolicyStatus2::get_updatesSuppressedTimes */ ,
    (void *) (INT_PTR) -1 /* IPolicyStatus2::get_downloadPreferenceGroupPolicy */ ,
    (void *) (INT_PTR) -1 /* IPolicyStatus2::get_packageCacheSizeLimitMBytes */ ,
    (void *) (INT_PTR) -1 /* IPolicyStatus2::get_packageCacheExpirationTimeDays */ ,
    (void *) (INT_PTR) -1 /* IPolicyStatus2::get_proxyMode */ ,
    (void *) (INT_PTR) -1 /* IPolicyStatus2::get_proxyPacUrl */ ,
    (void *) (INT_PTR) -1 /* IPolicyStatus2::get_proxyServer */ ,
    (void *) (INT_PTR) -1 /* IPolicyStatus2::get_effectivePolicyForAppInstalls */ ,
    (void *) (INT_PTR) -1 /* IPolicyStatus2::get_effectivePolicyForAppUpdates */ ,
    (void *) (INT_PTR) -1 /* IPolicyStatus2::get_targetVersionPrefix */ ,
    (void *) (INT_PTR) -1 /* IPolicyStatus2::get_isRollbackToTargetVersionAllowed */ ,
    (void *) (INT_PTR) -1 /* IPolicyStatus2::get_targetChannel */
};


static const PRPC_STUB_FUNCTION IPolicyStatus2_table[] =
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
    NdrStubCall2
};

CInterfaceStubVtbl _IPolicyStatus2StubVtbl =
{
    &IID_IPolicyStatus2,
    &IPolicyStatus2_ServerInfo,
    23,
    &IPolicyStatus2_table[-3],
    CStdStubBuffer_DELEGATING_METHODS
};


/* Object interface: IPolicyStatus3, ver. 0.0,
   GUID={0xC974F2DD,0xCFB8,0x4466,{0x8E,0x6D,0x96,0xED,0x90,0x1D,0xAA,0xCA}} */

#pragma code_seg(".orpc")
static const unsigned short IPolicyStatus3_FormatStringOffsetTable[] =
    {
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    (unsigned short) -1,
    114,
    3642,
    3678,
    3708,
    3744,
    3786,
    3822,
    3858,
    3894,
    3930,
    3966,
    4002,
    4044,
    4086,
    4128,
    4170,
    4212
    };

static const MIDL_STUBLESS_PROXY_INFO IPolicyStatus3_ProxyInfo =
    {
    &Object_StubDesc,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IPolicyStatus3_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IPolicyStatus3_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    google_update_idl__MIDL_ProcFormatString.Format,
    &IPolicyStatus3_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(24) _IPolicyStatus3ProxyVtbl = 
{
    &IPolicyStatus3_ProxyInfo,
    &IID_IPolicyStatus3,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    0 /* IDispatch::GetTypeInfoCount */ ,
    0 /* IDispatch::GetTypeInfo */ ,
    0 /* IDispatch::GetIDsOfNames */ ,
    0 /* IDispatch_Invoke_Proxy */ ,
    (void *) (INT_PTR) -1 /* IPolicyStatus2::get_updaterVersion */ ,
    (void *) (INT_PTR) -1 /* IPolicyStatus2::get_lastCheckedTime */ ,
    (void *) (INT_PTR) -1 /* IPolicyStatus2::refreshPolicies */ ,
    (void *) (INT_PTR) -1 /* IPolicyStatus2::get_lastCheckPeriodMinutes */ ,
    (void *) (INT_PTR) -1 /* IPolicyStatus2::get_updatesSuppressedTimes */ ,
    (void *) (INT_PTR) -1 /* IPolicyStatus2::get_downloadPreferenceGroupPolicy */ ,
    (void *) (INT_PTR) -1 /* IPolicyStatus2::get_packageCacheSizeLimitMBytes */ ,
    (void *) (INT_PTR) -1 /* IPolicyStatus2::get_packageCacheExpirationTimeDays */ ,
    (void *) (INT_PTR) -1 /* IPolicyStatus2::get_proxyMode */ ,
    (void *) (INT_PTR) -1 /* IPolicyStatus2::get_proxyPacUrl */ ,
    (void *) (INT_PTR) -1 /* IPolicyStatus2::get_proxyServer */ ,
    (void *) (INT_PTR) -1 /* IPolicyStatus2::get_effectivePolicyForAppInstalls */ ,
    (void *) (INT_PTR) -1 /* IPolicyStatus2::get_effectivePolicyForAppUpdates */ ,
    (void *) (INT_PTR) -1 /* IPolicyStatus2::get_targetVersionPrefix */ ,
    (void *) (INT_PTR) -1 /* IPolicyStatus2::get_isRollbackToTargetVersionAllowed */ ,
    (void *) (INT_PTR) -1 /* IPolicyStatus2::get_targetChannel */ ,
    (void *) (INT_PTR) -1 /* IPolicyStatus3::get_forceInstallApps */
};


static const PRPC_STUB_FUNCTION IPolicyStatus3_table[] =
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

CInterfaceStubVtbl _IPolicyStatus3StubVtbl =
{
    &IID_IPolicyStatus3,
    &IPolicyStatus3_ServerInfo,
    24,
    &IPolicyStatus3_table[-3],
    CStdStubBuffer_DELEGATING_METHODS
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
    1320
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
    4254
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
    4290,
    150,
    3678,
    1392,
    1428,
    4344,
    2994,
    4380,
    4422,
    4452,
    4482,
    4512,
    4542,
    4572,
    4602,
    4632,
    4674
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
    114,
    1356,
    78,
    4710,
    4752,
    4782,
    4818,
    4848,
    402,
    438
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
    2580,
    186,
    2652
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
    114,
    2580,
    2736
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
    2544,
    2580,
    78
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
    4878
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
    4926
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
    4998,
    5034,
    5076
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
    4998,
    5034,
    5076,
    5130
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


/* Standard interface: __MIDL_itf_google_update_idl_0000_0026, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: IProgressWndEvents, ver. 0.0,
   GUID={0x8504FB26,0xFC3E,0x4C1C,{0x9C,0x94,0x46,0xEC,0x93,0xE6,0xBA,0x63}} */

#pragma code_seg(".orpc")
static const unsigned short IProgressWndEvents_FormatStringOffsetTable[] =
    {
    5184,
    5214,
    5244,
    5274,
    5304,
    5334
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
    5184,
    5214,
    5370,
    5274,
    5406,
    5448,
    3678,
    5478,
    5508,
    5550
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
    5586
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
    5628,
    5670
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
    5712
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
    0x8010272, /* MIDL Version 8.1.626 */
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
    ( CInterfaceProxyVtbl *) &_IPolicyStatus2ProxyVtbl,
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
    ( CInterfaceProxyVtbl *) &_IPolicyStatusProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IPolicyStatus3ProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICredentialDialogProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IProcessLauncherProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IPolicyStatusValueProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IGoogleUpdateProxyVtbl,
    ( CInterfaceProxyVtbl *) &_ICurrentStateProxyVtbl,
    0
};

const CInterfaceStubVtbl * const _google_update_idl_StubVtblList[] = 
{
    ( CInterfaceStubVtbl *) &_IJobObserverStubVtbl,
    ( CInterfaceStubVtbl *) &_ICoCreateAsyncStatusStubVtbl,
    ( CInterfaceStubVtbl *) &_IAppVersionWebStubVtbl,
    ( CInterfaceStubVtbl *) &_IPolicyStatus2StubVtbl,
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
    ( CInterfaceStubVtbl *) &_IPolicyStatusStubVtbl,
    ( CInterfaceStubVtbl *) &_IPolicyStatus3StubVtbl,
    ( CInterfaceStubVtbl *) &_ICredentialDialogStubVtbl,
    ( CInterfaceStubVtbl *) &_IProcessLauncherStubVtbl,
    ( CInterfaceStubVtbl *) &_IPolicyStatusValueStubVtbl,
    ( CInterfaceStubVtbl *) &_IGoogleUpdateStubVtbl,
    ( CInterfaceStubVtbl *) &_ICurrentStateStubVtbl,
    0
};

PCInterfaceName const _google_update_idl_InterfaceNamesList[] = 
{
    "IJobObserver",
    "ICoCreateAsyncStatus",
    "IAppVersionWeb",
    "IPolicyStatus2",
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
    "IPolicyStatus",
    "IPolicyStatus3",
    "ICredentialDialog",
    "IProcessLauncher",
    "IPolicyStatusValue",
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
    &IID_IDispatch,
    &IID_IDispatch,
    0,
    0,
    &IID_IDispatch,
    0,
    &IID_IDispatch,
    0
};


#define _google_update_idl_CHECK_IID(n)	IID_GENERIC_CHECK_IID( _google_update_idl, pIID, n)

int __stdcall _google_update_idl_IID_Lookup( const IID * pIID, int * pIndex )
{
    IID_BS_LOOKUP_SETUP

    IID_BS_LOOKUP_INITIAL_TEST( _google_update_idl, 31, 16 )
    IID_BS_LOOKUP_NEXT_TEST( _google_update_idl, 8 )
    IID_BS_LOOKUP_NEXT_TEST( _google_update_idl, 4 )
    IID_BS_LOOKUP_NEXT_TEST( _google_update_idl, 2 )
    IID_BS_LOOKUP_NEXT_TEST( _google_update_idl, 1 )
    IID_BS_LOOKUP_RETURN_RESULT( _google_update_idl, 31, *pIndex )
    
}

const ExtendedProxyFileInfo google_update_idl_ProxyFileInfo = 
{
    (PCInterfaceProxyVtblList *) & _google_update_idl_ProxyVtblList,
    (PCInterfaceStubVtblList *) & _google_update_idl_StubVtblList,
    (const PCInterfaceName * ) & _google_update_idl_InterfaceNamesList,
    (const IID ** ) & _google_update_idl_BaseIIDList,
    & _google_update_idl_IID_Lookup, 
    31,
    2,
    0, /* table of [async_uuid] interfaces */
    0, /* Filler1 */
    0, /* Filler2 */
    0  /* Filler3 */
};
#if _MSC_VER >= 1200
#pragma warning(pop)
#endif


#endif /* !defined(_M_IA64) && !defined(_M_AMD64) && !defined(_ARM_) */

