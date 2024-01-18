

/* this ALWAYS GENERATED file contains the proxy stub code */


 /* File created by MIDL compiler version 8.xx.xxxx */
/* at a redacted point in time
 */
/* Compiler settings for ../../brave/chromium_src/chrome/elevation_service/elevation_service_idl.idl:
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


#include "elevation_service_idl.h"

#define TYPE_FORMAT_STRING_SIZE   69                                
#define PROC_FORMAT_STRING_SIZE   207                               
#define EXPR_FORMAT_STRING_SIZE   1                                 
#define TRANSMIT_AS_TABLE_SIZE    0            
#define WIRE_MARSHAL_TABLE_SIZE   1            

typedef struct _elevation_service_idl_MIDL_TYPE_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ TYPE_FORMAT_STRING_SIZE ];
    } elevation_service_idl_MIDL_TYPE_FORMAT_STRING;

typedef struct _elevation_service_idl_MIDL_PROC_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ PROC_FORMAT_STRING_SIZE ];
    } elevation_service_idl_MIDL_PROC_FORMAT_STRING;

typedef struct _elevation_service_idl_MIDL_EXPR_FORMAT_STRING
    {
    long          Pad;
    unsigned char  Format[ EXPR_FORMAT_STRING_SIZE ];
    } elevation_service_idl_MIDL_EXPR_FORMAT_STRING;


static const RPC_SYNTAX_IDENTIFIER  _RpcTransferSyntax_2_0 = 
{{0x8A885D04,0x1CEB,0x11C9,{0x9F,0xE8,0x08,0x00,0x2B,0x10,0x48,0x60}},{2,0}};

#if defined(_CONTROL_FLOW_GUARD_XFG)
#define XFG_TRAMPOLINES(ObjectType)\
NDR_SHAREABLE unsigned long ObjectType ## _UserSize_XFG(unsigned long * pFlags, unsigned long Offset, void * pObject)\
{\
return  ObjectType ## _UserSize(pFlags, Offset, (ObjectType *)pObject);\
}\
NDR_SHAREABLE unsigned char * ObjectType ## _UserMarshal_XFG(unsigned long * pFlags, unsigned char * pBuffer, void * pObject)\
{\
return ObjectType ## _UserMarshal(pFlags, pBuffer, (ObjectType *)pObject);\
}\
NDR_SHAREABLE unsigned char * ObjectType ## _UserUnmarshal_XFG(unsigned long * pFlags, unsigned char * pBuffer, void * pObject)\
{\
return ObjectType ## _UserUnmarshal(pFlags, pBuffer, (ObjectType *)pObject);\
}\
NDR_SHAREABLE void ObjectType ## _UserFree_XFG(unsigned long * pFlags, void * pObject)\
{\
ObjectType ## _UserFree(pFlags, (ObjectType *)pObject);\
}
#define XFG_TRAMPOLINES64(ObjectType)\
NDR_SHAREABLE unsigned long ObjectType ## _UserSize64_XFG(unsigned long * pFlags, unsigned long Offset, void * pObject)\
{\
return  ObjectType ## _UserSize64(pFlags, Offset, (ObjectType *)pObject);\
}\
NDR_SHAREABLE unsigned char * ObjectType ## _UserMarshal64_XFG(unsigned long * pFlags, unsigned char * pBuffer, void * pObject)\
{\
return ObjectType ## _UserMarshal64(pFlags, pBuffer, (ObjectType *)pObject);\
}\
NDR_SHAREABLE unsigned char * ObjectType ## _UserUnmarshal64_XFG(unsigned long * pFlags, unsigned char * pBuffer, void * pObject)\
{\
return ObjectType ## _UserUnmarshal64(pFlags, pBuffer, (ObjectType *)pObject);\
}\
NDR_SHAREABLE void ObjectType ## _UserFree64_XFG(unsigned long * pFlags, void * pObject)\
{\
ObjectType ## _UserFree64(pFlags, (ObjectType *)pObject);\
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
#define XFG_TRAMPOLINE_FPTR_DEPENDENT_SYMBOL(Symbol) Symbol ## _XFG
#else
#define XFG_TRAMPOLINES(ObjectType)
#define XFG_TRAMPOLINES64(ObjectType)
#define XFG_BIND_TRAMPOLINES(HandleType, ObjectType)
#define XFG_TRAMPOLINE_FPTR(Function) Function
#define XFG_TRAMPOLINE_FPTR_DEPENDENT_SYMBOL(Symbol) Symbol
#endif


extern const elevation_service_idl_MIDL_TYPE_FORMAT_STRING elevation_service_idl__MIDL_TypeFormatString;
extern const elevation_service_idl_MIDL_PROC_FORMAT_STRING elevation_service_idl__MIDL_ProcFormatString;
extern const elevation_service_idl_MIDL_EXPR_FORMAT_STRING elevation_service_idl__MIDL_ExprFormatString;

#ifdef __cplusplus
namespace {
#endif

extern const MIDL_STUB_DESC Object_StubDesc;
#ifdef __cplusplus
}
#endif


extern const MIDL_SERVER_INFO IElevator_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IElevator_ProxyInfo;

#ifdef __cplusplus
namespace {
#endif

extern const MIDL_STUB_DESC Object_StubDesc;
#ifdef __cplusplus
}
#endif


extern const MIDL_SERVER_INFO IElevatorChromium_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IElevatorChromium_ProxyInfo;

#ifdef __cplusplus
namespace {
#endif

extern const MIDL_STUB_DESC Object_StubDesc;
#ifdef __cplusplus
}
#endif


extern const MIDL_SERVER_INFO IElevatorChrome_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IElevatorChrome_ProxyInfo;

#ifdef __cplusplus
namespace {
#endif

extern const MIDL_STUB_DESC Object_StubDesc;
#ifdef __cplusplus
}
#endif


extern const MIDL_SERVER_INFO IElevatorChromeBeta_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IElevatorChromeBeta_ProxyInfo;

#ifdef __cplusplus
namespace {
#endif

extern const MIDL_STUB_DESC Object_StubDesc;
#ifdef __cplusplus
}
#endif


extern const MIDL_SERVER_INFO IElevatorChromeDev_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IElevatorChromeDev_ProxyInfo;

#ifdef __cplusplus
namespace {
#endif

extern const MIDL_STUB_DESC Object_StubDesc;
#ifdef __cplusplus
}
#endif


extern const MIDL_SERVER_INFO IElevatorChromeCanary_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IElevatorChromeCanary_ProxyInfo;

#ifdef __cplusplus
namespace {
#endif

extern const MIDL_STUB_DESC Object_StubDesc;
#ifdef __cplusplus
}
#endif


extern const MIDL_SERVER_INFO IElevatorDevelopment_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IElevatorDevelopment_ProxyInfo;


extern const USER_MARSHAL_ROUTINE_QUADRUPLE UserMarshalRoutines[ WIRE_MARSHAL_TABLE_SIZE ];

#if !defined(__RPC_WIN64__)
#error  Invalid build platform for this stub.
#endif

static const elevation_service_idl_MIDL_PROC_FORMAT_STRING elevation_service_idl__MIDL_ProcFormatString =
    {
        0,
        {

	/* Procedure RunRecoveryCRXElevated */

			0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/*  2 */	NdrFcLong( 0x0 ),	/* 0 */
/*  6 */	NdrFcShort( 0x3 ),	/* 3 */
/*  8 */	NdrFcShort( 0x40 ),	/* X64 Stack size/offset = 64 */
/* 10 */	NdrFcShort( 0x8 ),	/* 8 */
/* 12 */	NdrFcShort( 0x24 ),	/* 36 */
/* 14 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x7,		/* 7 */
/* 16 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 18 */	NdrFcShort( 0x0 ),	/* 0 */
/* 20 */	NdrFcShort( 0x0 ),	/* 0 */
/* 22 */	NdrFcShort( 0x0 ),	/* 0 */
/* 24 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter crx_path */

/* 26 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 28 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 30 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter browser_appid */

/* 32 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 34 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 36 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter browser_version */

/* 38 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 40 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 42 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter session_id */

/* 44 */	NdrFcShort( 0x10b ),	/* Flags:  must size, must free, in, simple ref, */
/* 46 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 48 */	NdrFcShort( 0x4 ),	/* Type Offset=4 */

	/* Parameter caller_proc_id */

/* 50 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 52 */	NdrFcShort( 0x28 ),	/* X64 Stack size/offset = 40 */
/* 54 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Parameter proc_handle */

/* 56 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 58 */	NdrFcShort( 0x30 ),	/* X64 Stack size/offset = 48 */
/* 60 */	0xb9,		/* FC_UINT3264 */
			0x0,		/* 0 */

	/* Return value */

/* 62 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 64 */	NdrFcShort( 0x38 ),	/* X64 Stack size/offset = 56 */
/* 66 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure EncryptData */

/* 68 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 70 */	NdrFcLong( 0x0 ),	/* 0 */
/* 74 */	NdrFcShort( 0x4 ),	/* 4 */
/* 76 */	NdrFcShort( 0x30 ),	/* X64 Stack size/offset = 48 */
/* 78 */	NdrFcShort( 0x6 ),	/* 6 */
/* 80 */	NdrFcShort( 0x24 ),	/* 36 */
/* 82 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x5,		/* 5 */
/* 84 */	0xa,		/* 10 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 86 */	NdrFcShort( 0x1 ),	/* 1 */
/* 88 */	NdrFcShort( 0x1 ),	/* 1 */
/* 90 */	NdrFcShort( 0x0 ),	/* 0 */
/* 92 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter protection_level */

/* 94 */	NdrFcShort( 0x48 ),	/* Flags:  in, base type, */
/* 96 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 98 */	0xd,		/* FC_ENUM16 */
			0x0,		/* 0 */

	/* Parameter plaintext */

/* 100 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 102 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 104 */	NdrFcShort( 0x24 ),	/* Type Offset=36 */

	/* Parameter ciphertext */

/* 106 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 108 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 110 */	NdrFcShort( 0x36 ),	/* Type Offset=54 */

	/* Parameter last_error */

/* 112 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 114 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 116 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 118 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 120 */	NdrFcShort( 0x28 ),	/* X64 Stack size/offset = 40 */
/* 122 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure DecryptData */

/* 124 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 126 */	NdrFcLong( 0x0 ),	/* 0 */
/* 130 */	NdrFcShort( 0x5 ),	/* 5 */
/* 132 */	NdrFcShort( 0x28 ),	/* X64 Stack size/offset = 40 */
/* 134 */	NdrFcShort( 0x0 ),	/* 0 */
/* 136 */	NdrFcShort( 0x24 ),	/* 36 */
/* 138 */	0x47,		/* Oi2 Flags:  srv must size, clt must size, has return, has ext, */
			0x4,		/* 4 */
/* 140 */	0xa,		/* 10 */
			0x7,		/* Ext Flags:  new corr desc, clt corr check, srv corr check, */
/* 142 */	NdrFcShort( 0x1 ),	/* 1 */
/* 144 */	NdrFcShort( 0x1 ),	/* 1 */
/* 146 */	NdrFcShort( 0x0 ),	/* 0 */
/* 148 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Parameter ciphertext */

/* 150 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 152 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 154 */	NdrFcShort( 0x24 ),	/* Type Offset=36 */

	/* Parameter plaintext */

/* 156 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 158 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 160 */	NdrFcShort( 0x36 ),	/* Type Offset=54 */

	/* Parameter last_error */

/* 162 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 164 */	NdrFcShort( 0x18 ),	/* X64 Stack size/offset = 24 */
/* 166 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 168 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 170 */	NdrFcShort( 0x20 ),	/* X64 Stack size/offset = 32 */
/* 172 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure InstallVPNServices */

/* 174 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 176 */	NdrFcLong( 0x0 ),	/* 0 */
/* 180 */	NdrFcShort( 0x6 ),	/* 6 */
/* 182 */	NdrFcShort( 0x10 ),	/* X64 Stack size/offset = 16 */
/* 184 */	NdrFcShort( 0x0 ),	/* 0 */
/* 186 */	NdrFcShort( 0x8 ),	/* 8 */
/* 188 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x1,		/* 1 */
/* 190 */	0xa,		/* 10 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 192 */	NdrFcShort( 0x0 ),	/* 0 */
/* 194 */	NdrFcShort( 0x0 ),	/* 0 */
/* 196 */	NdrFcShort( 0x0 ),	/* 0 */
/* 198 */	NdrFcShort( 0x0 ),	/* 0 */

	/* Return value */

/* 200 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 202 */	NdrFcShort( 0x8 ),	/* X64 Stack size/offset = 8 */
/* 204 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

			0x0
        }
    };

static const elevation_service_idl_MIDL_TYPE_FORMAT_STRING elevation_service_idl__MIDL_TypeFormatString =
    {
        0,
        {
			NdrFcShort( 0x0 ),	/* 0 */
/*  2 */	
			0x11, 0x8,	/* FC_RP [simple_pointer] */
/*  4 */	
			0x25,		/* FC_C_WSTRING */
			0x5c,		/* FC_PAD */
/*  6 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/*  8 */	0xb9,		/* FC_UINT3264 */
			0x5c,		/* FC_PAD */
/* 10 */	
			0x12, 0x0,	/* FC_UP */
/* 12 */	NdrFcShort( 0xe ),	/* Offset= 14 (26) */
/* 14 */	
			0x1b,		/* FC_CARRAY */
			0x1,		/* 1 */
/* 16 */	NdrFcShort( 0x2 ),	/* 2 */
/* 18 */	0x9,		/* Corr desc: FC_ULONG */
			0x0,		/*  */
/* 20 */	NdrFcShort( 0xfffc ),	/* -4 */
/* 22 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 24 */	0x6,		/* FC_SHORT */
			0x5b,		/* FC_END */
/* 26 */	
			0x17,		/* FC_CSTRUCT */
			0x3,		/* 3 */
/* 28 */	NdrFcShort( 0x8 ),	/* 8 */
/* 30 */	NdrFcShort( 0xfff0 ),	/* Offset= -16 (14) */
/* 32 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 34 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 36 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 38 */	NdrFcShort( 0x0 ),	/* 0 */
/* 40 */	NdrFcShort( 0x8 ),	/* 8 */
/* 42 */	NdrFcShort( 0x0 ),	/* 0 */
/* 44 */	NdrFcShort( 0xffde ),	/* Offset= -34 (10) */
/* 46 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 48 */	NdrFcShort( 0x6 ),	/* Offset= 6 (54) */
/* 50 */	
			0x13, 0x0,	/* FC_OP */
/* 52 */	NdrFcShort( 0xffe6 ),	/* Offset= -26 (26) */
/* 54 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 56 */	NdrFcShort( 0x0 ),	/* 0 */
/* 58 */	NdrFcShort( 0x8 ),	/* 8 */
/* 60 */	NdrFcShort( 0x0 ),	/* 0 */
/* 62 */	NdrFcShort( 0xfff4 ),	/* Offset= -12 (50) */
/* 64 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 66 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */

			0x0
        }
    };

XFG_TRAMPOLINES(BSTR)

static const USER_MARSHAL_ROUTINE_QUADRUPLE UserMarshalRoutines[ WIRE_MARSHAL_TABLE_SIZE ] = 
        {
            
            {
            (USER_MARSHAL_SIZING_ROUTINE)XFG_TRAMPOLINE_FPTR(BSTR_UserSize)
            ,(USER_MARSHAL_MARSHALLING_ROUTINE)XFG_TRAMPOLINE_FPTR(BSTR_UserMarshal)
            ,(USER_MARSHAL_UNMARSHALLING_ROUTINE)XFG_TRAMPOLINE_FPTR(BSTR_UserUnmarshal)
            ,(USER_MARSHAL_FREEING_ROUTINE)XFG_TRAMPOLINE_FPTR(BSTR_UserFree)
            
            }
            

        };



/* Standard interface: __MIDL_itf_elevation_service_idl_0000_0000, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00}} */


/* Object interface: IUnknown, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: IElevator, ver. 0.0,
   GUID={0x5A9A9462,0x2FA1,0x4FEB,{0xB7,0xF2,0xDF,0x3D,0x19,0x13,0x44,0x63}} */

#pragma code_seg(".orpc")
static const unsigned short IElevator_FormatStringOffsetTable[] =
    {
    0,
    68,
    124,
    174
    };

static const MIDL_STUBLESS_PROXY_INFO IElevator_ProxyInfo =
    {
    &Object_StubDesc,
    elevation_service_idl__MIDL_ProcFormatString.Format,
    &IElevator_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IElevator_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    elevation_service_idl__MIDL_ProcFormatString.Format,
    &IElevator_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(7) _IElevatorProxyVtbl = 
{
    &IElevator_ProxyInfo,
    &IID_IElevator,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* IElevator::RunRecoveryCRXElevated */ ,
    (void *) (INT_PTR) -1 /* IElevator::EncryptData */ ,
    (void *) (INT_PTR) -1 /* IElevator::DecryptData */ ,
    (void *) (INT_PTR) -1 /* IElevator::InstallVPNServices */
};

const CInterfaceStubVtbl _IElevatorStubVtbl =
{
    &IID_IElevator,
    &IElevator_ServerInfo,
    7,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
};


/* Object interface: IElevatorChromium, ver. 0.0,
   GUID={0x3218DA17,0x49C2,0x479A,{0x82,0x90,0x31,0x1D,0xBF,0xB8,0x64,0x90}} */

#pragma code_seg(".orpc")
static const unsigned short IElevatorChromium_FormatStringOffsetTable[] =
    {
    0,
    68,
    124,
    174,
    0
    };

static const MIDL_STUBLESS_PROXY_INFO IElevatorChromium_ProxyInfo =
    {
    &Object_StubDesc,
    elevation_service_idl__MIDL_ProcFormatString.Format,
    &IElevatorChromium_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IElevatorChromium_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    elevation_service_idl__MIDL_ProcFormatString.Format,
    &IElevatorChromium_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(7) _IElevatorChromiumProxyVtbl = 
{
    0,
    &IID_IElevatorChromium,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    0 /* forced delegation IElevator::RunRecoveryCRXElevated */ ,
    0 /* forced delegation IElevator::EncryptData */ ,
    0 /* forced delegation IElevator::DecryptData */ ,
    0 /* forced delegation IElevator::InstallVPNServices */
};


EXTERN_C DECLSPEC_SELECTANY const PRPC_STUB_FUNCTION IElevatorChromium_table[] =
{
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2
};

CInterfaceStubVtbl _IElevatorChromiumStubVtbl =
{
    &IID_IElevatorChromium,
    &IElevatorChromium_ServerInfo,
    7,
    &IElevatorChromium_table[-3],
    CStdStubBuffer_DELEGATING_METHODS
};


/* Object interface: IElevatorChrome, ver. 0.0,
   GUID={0xF396861E,0x0C8E,0x4C71,{0x82,0x56,0x2F,0xAE,0x6D,0x75,0x9C,0xE9}} */

#pragma code_seg(".orpc")
static const unsigned short IElevatorChrome_FormatStringOffsetTable[] =
    {
    0,
    68,
    124,
    174,
    0
    };

static const MIDL_STUBLESS_PROXY_INFO IElevatorChrome_ProxyInfo =
    {
    &Object_StubDesc,
    elevation_service_idl__MIDL_ProcFormatString.Format,
    &IElevatorChrome_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IElevatorChrome_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    elevation_service_idl__MIDL_ProcFormatString.Format,
    &IElevatorChrome_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(7) _IElevatorChromeProxyVtbl = 
{
    0,
    &IID_IElevatorChrome,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    0 /* forced delegation IElevator::RunRecoveryCRXElevated */ ,
    0 /* forced delegation IElevator::EncryptData */ ,
    0 /* forced delegation IElevator::DecryptData */ ,
    0 /* forced delegation IElevator::InstallVPNServices */
};


EXTERN_C DECLSPEC_SELECTANY const PRPC_STUB_FUNCTION IElevatorChrome_table[] =
{
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2
};

CInterfaceStubVtbl _IElevatorChromeStubVtbl =
{
    &IID_IElevatorChrome,
    &IElevatorChrome_ServerInfo,
    7,
    &IElevatorChrome_table[-3],
    CStdStubBuffer_DELEGATING_METHODS
};


/* Object interface: IElevatorChromeBeta, ver. 0.0,
   GUID={0x9EBAD7AC,0x6E1E,0x4A1C,{0xAA,0x85,0x1A,0x70,0xCA,0xDA,0x8D,0x82}} */

#pragma code_seg(".orpc")
static const unsigned short IElevatorChromeBeta_FormatStringOffsetTable[] =
    {
    0,
    68,
    124,
    174,
    0
    };

static const MIDL_STUBLESS_PROXY_INFO IElevatorChromeBeta_ProxyInfo =
    {
    &Object_StubDesc,
    elevation_service_idl__MIDL_ProcFormatString.Format,
    &IElevatorChromeBeta_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IElevatorChromeBeta_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    elevation_service_idl__MIDL_ProcFormatString.Format,
    &IElevatorChromeBeta_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(7) _IElevatorChromeBetaProxyVtbl = 
{
    0,
    &IID_IElevatorChromeBeta,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    0 /* forced delegation IElevator::RunRecoveryCRXElevated */ ,
    0 /* forced delegation IElevator::EncryptData */ ,
    0 /* forced delegation IElevator::DecryptData */ ,
    0 /* forced delegation IElevator::InstallVPNServices */
};


EXTERN_C DECLSPEC_SELECTANY const PRPC_STUB_FUNCTION IElevatorChromeBeta_table[] =
{
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2
};

CInterfaceStubVtbl _IElevatorChromeBetaStubVtbl =
{
    &IID_IElevatorChromeBeta,
    &IElevatorChromeBeta_ServerInfo,
    7,
    &IElevatorChromeBeta_table[-3],
    CStdStubBuffer_DELEGATING_METHODS
};


/* Object interface: IElevatorChromeDev, ver. 0.0,
   GUID={0x1E43C77B,0x48E6,0x4A4C,{0x9D,0xB2,0xC2,0x97,0x17,0x06,0xC2,0x55}} */

#pragma code_seg(".orpc")
static const unsigned short IElevatorChromeDev_FormatStringOffsetTable[] =
    {
    0,
    68,
    124,
    174,
    0
    };

static const MIDL_STUBLESS_PROXY_INFO IElevatorChromeDev_ProxyInfo =
    {
    &Object_StubDesc,
    elevation_service_idl__MIDL_ProcFormatString.Format,
    &IElevatorChromeDev_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IElevatorChromeDev_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    elevation_service_idl__MIDL_ProcFormatString.Format,
    &IElevatorChromeDev_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(7) _IElevatorChromeDevProxyVtbl = 
{
    0,
    &IID_IElevatorChromeDev,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    0 /* forced delegation IElevator::RunRecoveryCRXElevated */ ,
    0 /* forced delegation IElevator::EncryptData */ ,
    0 /* forced delegation IElevator::DecryptData */ ,
    0 /* forced delegation IElevator::InstallVPNServices */
};


EXTERN_C DECLSPEC_SELECTANY const PRPC_STUB_FUNCTION IElevatorChromeDev_table[] =
{
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2
};

CInterfaceStubVtbl _IElevatorChromeDevStubVtbl =
{
    &IID_IElevatorChromeDev,
    &IElevatorChromeDev_ServerInfo,
    7,
    &IElevatorChromeDev_table[-3],
    CStdStubBuffer_DELEGATING_METHODS
};


/* Object interface: IElevatorChromeCanary, ver. 0.0,
   GUID={0x1DB2116F,0x71B7,0x49F0,{0x89,0x70,0x33,0xB1,0xDA,0xCF,0xB0,0x72}} */

#pragma code_seg(".orpc")
static const unsigned short IElevatorChromeCanary_FormatStringOffsetTable[] =
    {
    0,
    68,
    124,
    174,
    0
    };

static const MIDL_STUBLESS_PROXY_INFO IElevatorChromeCanary_ProxyInfo =
    {
    &Object_StubDesc,
    elevation_service_idl__MIDL_ProcFormatString.Format,
    &IElevatorChromeCanary_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IElevatorChromeCanary_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    elevation_service_idl__MIDL_ProcFormatString.Format,
    &IElevatorChromeCanary_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(7) _IElevatorChromeCanaryProxyVtbl = 
{
    0,
    &IID_IElevatorChromeCanary,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    0 /* forced delegation IElevator::RunRecoveryCRXElevated */ ,
    0 /* forced delegation IElevator::EncryptData */ ,
    0 /* forced delegation IElevator::DecryptData */ ,
    0 /* forced delegation IElevator::InstallVPNServices */
};


EXTERN_C DECLSPEC_SELECTANY const PRPC_STUB_FUNCTION IElevatorChromeCanary_table[] =
{
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2
};

CInterfaceStubVtbl _IElevatorChromeCanaryStubVtbl =
{
    &IID_IElevatorChromeCanary,
    &IElevatorChromeCanary_ServerInfo,
    7,
    &IElevatorChromeCanary_table[-3],
    CStdStubBuffer_DELEGATING_METHODS
};


/* Object interface: IElevatorDevelopment, ver. 0.0,
   GUID={0x17239BF1,0xA1DC,0x4642,{0x84,0x6C,0x1B,0xAC,0x85,0xF9,0x6A,0x10}} */

#pragma code_seg(".orpc")
static const unsigned short IElevatorDevelopment_FormatStringOffsetTable[] =
    {
    0,
    68,
    124,
    174,
    0
    };

static const MIDL_STUBLESS_PROXY_INFO IElevatorDevelopment_ProxyInfo =
    {
    &Object_StubDesc,
    elevation_service_idl__MIDL_ProcFormatString.Format,
    &IElevatorDevelopment_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IElevatorDevelopment_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    elevation_service_idl__MIDL_ProcFormatString.Format,
    &IElevatorDevelopment_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(7) _IElevatorDevelopmentProxyVtbl = 
{
    0,
    &IID_IElevatorDevelopment,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    0 /* forced delegation IElevator::RunRecoveryCRXElevated */ ,
    0 /* forced delegation IElevator::EncryptData */ ,
    0 /* forced delegation IElevator::DecryptData */ ,
    0 /* forced delegation IElevator::InstallVPNServices */
};


EXTERN_C DECLSPEC_SELECTANY const PRPC_STUB_FUNCTION IElevatorDevelopment_table[] =
{
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2,
    NdrStubCall2
};

CInterfaceStubVtbl _IElevatorDevelopmentStubVtbl =
{
    &IID_IElevatorDevelopment,
    &IElevatorDevelopment_ServerInfo,
    7,
    &IElevatorDevelopment_table[-3],
    CStdStubBuffer_DELEGATING_METHODS
};

#ifdef __cplusplus
namespace {
#endif
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
    elevation_service_idl__MIDL_TypeFormatString.Format,
    1, /* -error bounds_check flag */
    0x50002, /* Ndr library version */
    0,
    0x8010274, /* MIDL Version 8.1.628 */
    0,
    UserMarshalRoutines,
    0,  /* notify & notify_flag routine table */
    0x1, /* MIDL flag */
    0, /* cs routines */
    0,   /* proxy/server info */
    0
    };
#ifdef __cplusplus
}
#endif

const CInterfaceProxyVtbl * const _elevation_service_idl_ProxyVtblList[] = 
{
    ( CInterfaceProxyVtbl *) &_IElevatorChromiumProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IElevatorChromeProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IElevatorProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IElevatorChromeCanaryProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IElevatorChromeDevProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IElevatorChromeBetaProxyVtbl,
    ( CInterfaceProxyVtbl *) &_IElevatorDevelopmentProxyVtbl,
    0
};

const CInterfaceStubVtbl * const _elevation_service_idl_StubVtblList[] = 
{
    ( CInterfaceStubVtbl *) &_IElevatorChromiumStubVtbl,
    ( CInterfaceStubVtbl *) &_IElevatorChromeStubVtbl,
    ( CInterfaceStubVtbl *) &_IElevatorStubVtbl,
    ( CInterfaceStubVtbl *) &_IElevatorChromeCanaryStubVtbl,
    ( CInterfaceStubVtbl *) &_IElevatorChromeDevStubVtbl,
    ( CInterfaceStubVtbl *) &_IElevatorChromeBetaStubVtbl,
    ( CInterfaceStubVtbl *) &_IElevatorDevelopmentStubVtbl,
    0
};

PCInterfaceName const _elevation_service_idl_InterfaceNamesList[] = 
{
    "IElevatorChromium",
    "IElevatorChrome",
    "IElevator",
    "IElevatorChromeCanary",
    "IElevatorChromeDev",
    "IElevatorChromeBeta",
    "IElevatorDevelopment",
    0
};

const IID *  const _elevation_service_idl_BaseIIDList[] = 
{
    &IID_IElevator,   /* forced */
    &IID_IElevator,   /* forced */
    0,
    &IID_IElevator,   /* forced */
    &IID_IElevator,   /* forced */
    &IID_IElevator,   /* forced */
    &IID_IElevator,   /* forced */
    0
};


#define _elevation_service_idl_CHECK_IID(n)	IID_GENERIC_CHECK_IID( _elevation_service_idl, pIID, n)

int __stdcall _elevation_service_idl_IID_Lookup( const IID * pIID, int * pIndex )
{
    IID_BS_LOOKUP_SETUP

    IID_BS_LOOKUP_INITIAL_TEST( _elevation_service_idl, 7, 4 )
    IID_BS_LOOKUP_NEXT_TEST( _elevation_service_idl, 2 )
    IID_BS_LOOKUP_NEXT_TEST( _elevation_service_idl, 1 )
    IID_BS_LOOKUP_RETURN_RESULT( _elevation_service_idl, 7, *pIndex )
    
}

EXTERN_C const ExtendedProxyFileInfo elevation_service_idl_ProxyFileInfo = 
{
    (PCInterfaceProxyVtblList *) & _elevation_service_idl_ProxyVtblList,
    (PCInterfaceStubVtblList *) & _elevation_service_idl_StubVtblList,
    (const PCInterfaceName * ) & _elevation_service_idl_InterfaceNamesList,
    (const IID ** ) & _elevation_service_idl_BaseIIDList,
    & _elevation_service_idl_IID_Lookup, 
    7,
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

