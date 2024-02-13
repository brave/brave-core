

/* this ALWAYS GENERATED file contains the IIDs and CLSIDs */

/* link this file in with the server and any clients */


 /* File created by MIDL compiler version 8.xx.xxxx */
/* at a redacted point in time
 */
/* Compiler settings for ../../brave/chromium_src/chrome/elevation_service/elevation_service_idl.idl:
    Oicf, W1, Zp8, env=Win64 (32b run), target_arch=ARM64 8.01.0628 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#pragma warning( disable: 4049 )  /* more than 64k source lines */


#ifdef __cplusplus
extern "C"{
#endif 


#include <rpc.h>
#include <rpcndr.h>

#ifdef _MIDL_USE_GUIDDEF_

#ifndef INITGUID
#define INITGUID
#include <guiddef.h>
#undef INITGUID
#else
#include <guiddef.h>
#endif

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        DEFINE_GUID(name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8)

#else // !_MIDL_USE_GUIDDEF_

#ifndef __IID_DEFINED__
#define __IID_DEFINED__

typedef struct _IID
{
    unsigned long x;
    unsigned short s1;
    unsigned short s2;
    unsigned char  c[8];
} IID;

#endif // __IID_DEFINED__

#ifndef CLSID_DEFINED
#define CLSID_DEFINED
typedef IID CLSID;
#endif // CLSID_DEFINED

#define MIDL_DEFINE_GUID(type,name,l,w1,w2,b1,b2,b3,b4,b5,b6,b7,b8) \
        EXTERN_C __declspec(selectany) const type name = {l,w1,w2,{b1,b2,b3,b4,b5,b6,b7,b8}}

#endif // !_MIDL_USE_GUIDDEF_

MIDL_DEFINE_GUID(IID, IID_IElevator,0x5A9A9462,0x2FA1,0x4FEB,0xB7,0xF2,0xDF,0x3D,0x19,0x13,0x44,0x63);


MIDL_DEFINE_GUID(IID, IID_IElevatorChromium,0x3218DA17,0x49C2,0x479A,0x82,0x90,0x31,0x1D,0xBF,0xB8,0x64,0x90);


MIDL_DEFINE_GUID(IID, IID_IElevatorChrome,0xF396861E,0x0C8E,0x4C71,0x82,0x56,0x2F,0xAE,0x6D,0x75,0x9C,0xE9);


MIDL_DEFINE_GUID(IID, IID_IElevatorChromeBeta,0x9EBAD7AC,0x6E1E,0x4A1C,0xAA,0x85,0x1A,0x70,0xCA,0xDA,0x8D,0x82);


MIDL_DEFINE_GUID(IID, IID_IElevatorChromeDev,0x1E43C77B,0x48E6,0x4A4C,0x9D,0xB2,0xC2,0x97,0x17,0x06,0xC2,0x55);


MIDL_DEFINE_GUID(IID, IID_IElevatorChromeCanary,0x1DB2116F,0x71B7,0x49F0,0x89,0x70,0x33,0xB1,0xDA,0xCF,0xB0,0x72);


MIDL_DEFINE_GUID(IID, IID_IElevatorDevelopment,0x17239BF1,0xA1DC,0x4642,0x84,0x6C,0x1B,0xAC,0x85,0xF9,0x6A,0x10);


MIDL_DEFINE_GUID(IID, LIBID_ElevatorLib,0xC3B01C4D,0xFBD4,0x4E65,0x88,0xAD,0x09,0x72,0xD7,0x58,0x08,0xC2);

#undef MIDL_DEFINE_GUID

#ifdef __cplusplus
}
#endif



