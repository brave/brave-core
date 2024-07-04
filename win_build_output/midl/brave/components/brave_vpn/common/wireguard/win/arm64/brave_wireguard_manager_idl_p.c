

/* this ALWAYS GENERATED file contains the proxy stub code */


 /* File created by MIDL compiler version 8.xx.xxxx */
/* at a redacted point in time
 */
/* Compiler settings for ../../brave/components/brave_vpn/common/wireguard/win/brave_wireguard_manager_idl.idl:
    Oicf, W1, Zp8, env=Win64 (32b run), target_arch=ARM64 8.01.0628 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#if defined(_M_ARM64)


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


#include "brave_wireguard_manager_idl.h"

#define TYPE_FORMAT_STRING_SIZE   61                                
#define PROC_FORMAT_STRING_SIZE   169                               
#define EXPR_FORMAT_STRING_SIZE   1                                 
#define TRANSMIT_AS_TABLE_SIZE    0            
#define WIRE_MARSHAL_TABLE_SIZE   1            

typedef struct _brave_wireguard_manager_idl_MIDL_TYPE_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ TYPE_FORMAT_STRING_SIZE ];
    } brave_wireguard_manager_idl_MIDL_TYPE_FORMAT_STRING;

typedef struct _brave_wireguard_manager_idl_MIDL_PROC_FORMAT_STRING
    {
    short          Pad;
    unsigned char  Format[ PROC_FORMAT_STRING_SIZE ];
    } brave_wireguard_manager_idl_MIDL_PROC_FORMAT_STRING;

typedef struct _brave_wireguard_manager_idl_MIDL_EXPR_FORMAT_STRING
    {
    long          Pad;
    unsigned char  Format[ EXPR_FORMAT_STRING_SIZE ];
    } brave_wireguard_manager_idl_MIDL_EXPR_FORMAT_STRING;


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


extern const brave_wireguard_manager_idl_MIDL_TYPE_FORMAT_STRING brave_wireguard_manager_idl__MIDL_TypeFormatString;
extern const brave_wireguard_manager_idl_MIDL_PROC_FORMAT_STRING brave_wireguard_manager_idl__MIDL_ProcFormatString;
extern const brave_wireguard_manager_idl_MIDL_EXPR_FORMAT_STRING brave_wireguard_manager_idl__MIDL_ExprFormatString;

#ifdef __cplusplus
namespace {
#endif

extern const MIDL_STUB_DESC Object_StubDesc;
#ifdef __cplusplus
}
#endif


extern const MIDL_SERVER_INFO IBraveVpnWireguardManager_ServerInfo;
extern const MIDL_STUBLESS_PROXY_INFO IBraveVpnWireguardManager_ProxyInfo;


extern const USER_MARSHAL_ROUTINE_QUADRUPLE UserMarshalRoutines[ WIRE_MARSHAL_TABLE_SIZE ];

#if !defined(__RPC_ARM64__)
#error  Invalid build platform for this stub.
#endif

static const brave_wireguard_manager_idl_MIDL_PROC_FORMAT_STRING brave_wireguard_manager_idl__MIDL_ProcFormatString =
    {
        0,
        {

	/* Procedure EnableVpn */

			0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/*  2 */	NdrFcLong( 0x0 ),	/* 0 */
/*  6 */	NdrFcShort( 0x3 ),	/* 3 */
/*  8 */	NdrFcShort( 0x38 ),	/* ARM64 Stack size/offset = 56 */
/* 10 */	NdrFcShort( 0x0 ),	/* 0 */
/* 12 */	NdrFcShort( 0x24 ),	/* 36 */
/* 14 */	0x46,		/* Oi2 Flags:  clt must size, has return, has ext, */
			0x6,		/* 6 */
/* 16 */	0x12,		/* 18 */
			0x5,		/* Ext Flags:  new corr desc, srv corr check, */
/* 18 */	NdrFcShort( 0x0 ),	/* 0 */
/* 20 */	NdrFcShort( 0x1 ),	/* 1 */
/* 22 */	NdrFcShort( 0x0 ),	/* 0 */
/* 24 */	NdrFcShort( 0x6 ),	/* 6 */
/* 26 */	0x6,		/* 6 */
			0x80,		/* 128 */
/* 28 */	0x81,		/* 129 */
			0x82,		/* 130 */
/* 30 */	0x83,		/* 131 */
			0x84,		/* 132 */
/* 32 */	0x85,		/* 133 */
			0x0,		/* 0 */

	/* Parameter public_key */

/* 34 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 36 */	NdrFcShort( 0x8 ),	/* ARM64 Stack size/offset = 8 */
/* 38 */	NdrFcShort( 0x1c ),	/* Type Offset=28 */

	/* Parameter private_key */

/* 40 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 42 */	NdrFcShort( 0x10 ),	/* ARM64 Stack size/offset = 16 */
/* 44 */	NdrFcShort( 0x1c ),	/* Type Offset=28 */

	/* Parameter address */

/* 46 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 48 */	NdrFcShort( 0x18 ),	/* ARM64 Stack size/offset = 24 */
/* 50 */	NdrFcShort( 0x1c ),	/* Type Offset=28 */

	/* Parameter endpoint */

/* 52 */	NdrFcShort( 0x8b ),	/* Flags:  must size, must free, in, by val, */
/* 54 */	NdrFcShort( 0x20 ),	/* ARM64 Stack size/offset = 32 */
/* 56 */	NdrFcShort( 0x1c ),	/* Type Offset=28 */

	/* Parameter last_error */

/* 58 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 60 */	NdrFcShort( 0x28 ),	/* ARM64 Stack size/offset = 40 */
/* 62 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 64 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 66 */	NdrFcShort( 0x30 ),	/* ARM64 Stack size/offset = 48 */
/* 68 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure DisableVpn */

/* 70 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 72 */	NdrFcLong( 0x0 ),	/* 0 */
/* 76 */	NdrFcShort( 0x4 ),	/* 4 */
/* 78 */	NdrFcShort( 0x18 ),	/* ARM64 Stack size/offset = 24 */
/* 80 */	NdrFcShort( 0x0 ),	/* 0 */
/* 82 */	NdrFcShort( 0x24 ),	/* 36 */
/* 84 */	0x44,		/* Oi2 Flags:  has return, has ext, */
			0x2,		/* 2 */
/* 86 */	0xe,		/* 14 */
			0x1,		/* Ext Flags:  new corr desc, */
/* 88 */	NdrFcShort( 0x0 ),	/* 0 */
/* 90 */	NdrFcShort( 0x0 ),	/* 0 */
/* 92 */	NdrFcShort( 0x0 ),	/* 0 */
/* 94 */	NdrFcShort( 0x2 ),	/* 2 */
/* 96 */	0x2,		/* 2 */
			0x80,		/* 128 */
/* 98 */	0x81,		/* 129 */
			0x0,		/* 0 */

	/* Parameter last_error */

/* 100 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 102 */	NdrFcShort( 0x8 ),	/* ARM64 Stack size/offset = 8 */
/* 104 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 106 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 108 */	NdrFcShort( 0x10 ),	/* ARM64 Stack size/offset = 16 */
/* 110 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Procedure GenerateKeypair */

/* 112 */	0x33,		/* FC_AUTO_HANDLE */
			0x6c,		/* Old Flags:  object, Oi2 */
/* 114 */	NdrFcLong( 0x0 ),	/* 0 */
/* 118 */	NdrFcShort( 0x5 ),	/* 5 */
/* 120 */	NdrFcShort( 0x28 ),	/* ARM64 Stack size/offset = 40 */
/* 122 */	NdrFcShort( 0x0 ),	/* 0 */
/* 124 */	NdrFcShort( 0x24 ),	/* 36 */
/* 126 */	0x45,		/* Oi2 Flags:  srv must size, has return, has ext, */
			0x4,		/* 4 */
/* 128 */	0x10,		/* 16 */
			0x3,		/* Ext Flags:  new corr desc, clt corr check, */
/* 130 */	NdrFcShort( 0x1 ),	/* 1 */
/* 132 */	NdrFcShort( 0x0 ),	/* 0 */
/* 134 */	NdrFcShort( 0x0 ),	/* 0 */
/* 136 */	NdrFcShort( 0x4 ),	/* 4 */
/* 138 */	0x4,		/* 4 */
			0x80,		/* 128 */
/* 140 */	0x81,		/* 129 */
			0x82,		/* 130 */
/* 142 */	0x83,		/* 131 */
			0x0,		/* 0 */

	/* Parameter public_key */

/* 144 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 146 */	NdrFcShort( 0x8 ),	/* ARM64 Stack size/offset = 8 */
/* 148 */	NdrFcShort( 0x32 ),	/* Type Offset=50 */

	/* Parameter private_key */

/* 150 */	NdrFcShort( 0x2113 ),	/* Flags:  must size, must free, out, simple ref, srv alloc size=8 */
/* 152 */	NdrFcShort( 0x10 ),	/* ARM64 Stack size/offset = 16 */
/* 154 */	NdrFcShort( 0x32 ),	/* Type Offset=50 */

	/* Parameter last_error */

/* 156 */	NdrFcShort( 0x2150 ),	/* Flags:  out, base type, simple ref, srv alloc size=8 */
/* 158 */	NdrFcShort( 0x18 ),	/* ARM64 Stack size/offset = 24 */
/* 160 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

	/* Return value */

/* 162 */	NdrFcShort( 0x70 ),	/* Flags:  out, return, base type, */
/* 164 */	NdrFcShort( 0x20 ),	/* ARM64 Stack size/offset = 32 */
/* 166 */	0x8,		/* FC_LONG */
			0x0,		/* 0 */

			0x0
        }
    };

static const brave_wireguard_manager_idl_MIDL_TYPE_FORMAT_STRING brave_wireguard_manager_idl__MIDL_TypeFormatString =
    {
        0,
        {
			NdrFcShort( 0x0 ),	/* 0 */
/*  2 */	
			0x12, 0x0,	/* FC_UP */
/*  4 */	NdrFcShort( 0xe ),	/* Offset= 14 (18) */
/*  6 */	
			0x1b,		/* FC_CARRAY */
			0x1,		/* 1 */
/*  8 */	NdrFcShort( 0x2 ),	/* 2 */
/* 10 */	0x9,		/* Corr desc: FC_ULONG */
			0x0,		/*  */
/* 12 */	NdrFcShort( 0xfffc ),	/* -4 */
/* 14 */	NdrFcShort( 0x1 ),	/* Corr flags:  early, */
/* 16 */	0x6,		/* FC_SHORT */
			0x5b,		/* FC_END */
/* 18 */	
			0x17,		/* FC_CSTRUCT */
			0x3,		/* 3 */
/* 20 */	NdrFcShort( 0x8 ),	/* 8 */
/* 22 */	NdrFcShort( 0xfff0 ),	/* Offset= -16 (6) */
/* 24 */	0x8,		/* FC_LONG */
			0x8,		/* FC_LONG */
/* 26 */	0x5c,		/* FC_PAD */
			0x5b,		/* FC_END */
/* 28 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 30 */	NdrFcShort( 0x0 ),	/* 0 */
/* 32 */	NdrFcShort( 0x8 ),	/* 8 */
/* 34 */	NdrFcShort( 0x0 ),	/* 0 */
/* 36 */	NdrFcShort( 0xffde ),	/* Offset= -34 (2) */
/* 38 */	
			0x11, 0xc,	/* FC_RP [alloced_on_stack] [simple_pointer] */
/* 40 */	0x8,		/* FC_LONG */
			0x5c,		/* FC_PAD */
/* 42 */	
			0x11, 0x4,	/* FC_RP [alloced_on_stack] */
/* 44 */	NdrFcShort( 0x6 ),	/* Offset= 6 (50) */
/* 46 */	
			0x13, 0x0,	/* FC_OP */
/* 48 */	NdrFcShort( 0xffe2 ),	/* Offset= -30 (18) */
/* 50 */	0xb4,		/* FC_USER_MARSHAL */
			0x83,		/* 131 */
/* 52 */	NdrFcShort( 0x0 ),	/* 0 */
/* 54 */	NdrFcShort( 0x8 ),	/* 8 */
/* 56 */	NdrFcShort( 0x0 ),	/* 0 */
/* 58 */	NdrFcShort( 0xfff4 ),	/* Offset= -12 (46) */

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



/* Object interface: IUnknown, ver. 0.0,
   GUID={0x00000000,0x0000,0x0000,{0xC0,0x00,0x00,0x00,0x00,0x00,0x00,0x46}} */


/* Object interface: IBraveVpnWireguardManager, ver. 0.0,
   GUID={0x6D319801,0x690B,0x441E,{0x8C,0x94,0x5C,0x18,0xD8,0xE7,0xE9,0xD7}} */

#pragma code_seg(".orpc")
static const unsigned short IBraveVpnWireguardManager_FormatStringOffsetTable[] =
    {
    0,
    70,
    112
    };

static const MIDL_STUBLESS_PROXY_INFO IBraveVpnWireguardManager_ProxyInfo =
    {
    &Object_StubDesc,
    brave_wireguard_manager_idl__MIDL_ProcFormatString.Format,
    &IBraveVpnWireguardManager_FormatStringOffsetTable[-3],
    0,
    0,
    0
    };


static const MIDL_SERVER_INFO IBraveVpnWireguardManager_ServerInfo = 
    {
    &Object_StubDesc,
    0,
    brave_wireguard_manager_idl__MIDL_ProcFormatString.Format,
    &IBraveVpnWireguardManager_FormatStringOffsetTable[-3],
    0,
    0,
    0,
    0};
CINTERFACE_PROXY_VTABLE(6) _IBraveVpnWireguardManagerProxyVtbl = 
{
    &IBraveVpnWireguardManager_ProxyInfo,
    &IID_IBraveVpnWireguardManager,
    IUnknown_QueryInterface_Proxy,
    IUnknown_AddRef_Proxy,
    IUnknown_Release_Proxy ,
    (void *) (INT_PTR) -1 /* IBraveVpnWireguardManager::EnableVpn */ ,
    (void *) (INT_PTR) -1 /* IBraveVpnWireguardManager::DisableVpn */ ,
    (void *) (INT_PTR) -1 /* IBraveVpnWireguardManager::GenerateKeypair */
};

const CInterfaceStubVtbl _IBraveVpnWireguardManagerStubVtbl =
{
    &IID_IBraveVpnWireguardManager,
    &IBraveVpnWireguardManager_ServerInfo,
    6,
    0, /* pure interpreted */
    CStdStubBuffer_METHODS
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
    brave_wireguard_manager_idl__MIDL_TypeFormatString.Format,
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

const CInterfaceProxyVtbl * const _brave_wireguard_manager_idl_ProxyVtblList[] = 
{
    ( CInterfaceProxyVtbl *) &_IBraveVpnWireguardManagerProxyVtbl,
    0
};

const CInterfaceStubVtbl * const _brave_wireguard_manager_idl_StubVtblList[] = 
{
    ( CInterfaceStubVtbl *) &_IBraveVpnWireguardManagerStubVtbl,
    0
};

PCInterfaceName const _brave_wireguard_manager_idl_InterfaceNamesList[] = 
{
    "IBraveVpnWireguardManager",
    0
};


#define _brave_wireguard_manager_idl_CHECK_IID(n)	IID_GENERIC_CHECK_IID( _brave_wireguard_manager_idl, pIID, n)

int __stdcall _brave_wireguard_manager_idl_IID_Lookup( const IID * pIID, int * pIndex )
{
    
    if(!_brave_wireguard_manager_idl_CHECK_IID(0))
        {
        *pIndex = 0;
        return 1;
        }

    return 0;
}

EXTERN_C const ExtendedProxyFileInfo brave_wireguard_manager_idl_ProxyFileInfo = 
{
    (PCInterfaceProxyVtblList *) & _brave_wireguard_manager_idl_ProxyVtblList,
    (PCInterfaceStubVtblList *) & _brave_wireguard_manager_idl_StubVtblList,
    (const PCInterfaceName * ) & _brave_wireguard_manager_idl_InterfaceNamesList,
    0, /* no delegation */
    & _brave_wireguard_manager_idl_IID_Lookup, 
    1,
    2,
    0, /* table of [async_uuid] interfaces */
    0, /* Filler1 */
    0, /* Filler2 */
    0  /* Filler3 */
};
#if _MSC_VER >= 1200
#pragma warning(pop)
#endif


#endif /* defined(_M_ARM64) */

