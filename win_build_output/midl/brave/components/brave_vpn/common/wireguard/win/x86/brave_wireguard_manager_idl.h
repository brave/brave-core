

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.xx.xxxx */
/* at a redacted point in time
 */
/* Compiler settings for ../../brave/components/brave_vpn/common/wireguard/win/brave_wireguard_manager_idl.idl:
    Oicf, W1, Zp8, env=Win32 (32b run), target_arch=X86 8.xx.xxxx 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif /* __RPCNDR_H_VERSION__ */

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __brave_wireguard_manager_idl_h__
#define __brave_wireguard_manager_idl_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#ifndef DECLSPEC_XFGVIRT
#if defined(_CONTROL_FLOW_GUARD_XFG)
#define DECLSPEC_XFGVIRT(base, func) __declspec(xfg_virtual(base, func))
#else
#define DECLSPEC_XFGVIRT(base, func)
#endif
#endif

/* Forward Declarations */ 

#ifndef __IBraveVpnWireguardManager_FWD_DEFINED__
#define __IBraveVpnWireguardManager_FWD_DEFINED__
typedef interface IBraveVpnWireguardManager IBraveVpnWireguardManager;

#endif 	/* __IBraveVpnWireguardManager_FWD_DEFINED__ */


#ifndef __IBraveVpnWireguardManager_FWD_DEFINED__
#define __IBraveVpnWireguardManager_FWD_DEFINED__
typedef interface IBraveVpnWireguardManager IBraveVpnWireguardManager;

#endif 	/* __IBraveVpnWireguardManager_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __IBraveVpnWireguardManager_INTERFACE_DEFINED__
#define __IBraveVpnWireguardManager_INTERFACE_DEFINED__

/* interface IBraveVpnWireguardManager */
/* [unique][helpstring][uuid][oleautomation][object] */ 


EXTERN_C const IID IID_IBraveVpnWireguardManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("6D319801-690B-441E-8C94-5C18D8E7E9D7")
    IBraveVpnWireguardManager : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE EnableVpn( 
            /* [in] */ BSTR public_key,
            /* [in] */ BSTR private_key,
            /* [in] */ BSTR address,
            /* [in] */ BSTR endpoint,
            /* [out] */ DWORD *last_error) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableVpn( 
            /* [out] */ DWORD *last_error) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GenerateKeypair( 
            /* [out] */ BSTR *public_key,
            /* [out] */ BSTR *private_key,
            /* [out] */ DWORD *last_error) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IBraveVpnWireguardManagerVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBraveVpnWireguardManager * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBraveVpnWireguardManager * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBraveVpnWireguardManager * This);
        
        DECLSPEC_XFGVIRT(IBraveVpnWireguardManager, EnableVpn)
        HRESULT ( STDMETHODCALLTYPE *EnableVpn )( 
            IBraveVpnWireguardManager * This,
            /* [in] */ BSTR public_key,
            /* [in] */ BSTR private_key,
            /* [in] */ BSTR address,
            /* [in] */ BSTR endpoint,
            /* [out] */ DWORD *last_error);
        
        DECLSPEC_XFGVIRT(IBraveVpnWireguardManager, DisableVpn)
        HRESULT ( STDMETHODCALLTYPE *DisableVpn )( 
            IBraveVpnWireguardManager * This,
            /* [out] */ DWORD *last_error);
        
        DECLSPEC_XFGVIRT(IBraveVpnWireguardManager, GenerateKeypair)
        HRESULT ( STDMETHODCALLTYPE *GenerateKeypair )( 
            IBraveVpnWireguardManager * This,
            /* [out] */ BSTR *public_key,
            /* [out] */ BSTR *private_key,
            /* [out] */ DWORD *last_error);
        
        END_INTERFACE
    } IBraveVpnWireguardManagerVtbl;

    interface IBraveVpnWireguardManager
    {
        CONST_VTBL struct IBraveVpnWireguardManagerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBraveVpnWireguardManager_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IBraveVpnWireguardManager_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IBraveVpnWireguardManager_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IBraveVpnWireguardManager_EnableVpn(This,public_key,private_key,address,endpoint,last_error)	\
    ( (This)->lpVtbl -> EnableVpn(This,public_key,private_key,address,endpoint,last_error) ) 

#define IBraveVpnWireguardManager_DisableVpn(This,last_error)	\
    ( (This)->lpVtbl -> DisableVpn(This,last_error) ) 

#define IBraveVpnWireguardManager_GenerateKeypair(This,public_key,private_key,last_error)	\
    ( (This)->lpVtbl -> GenerateKeypair(This,public_key,private_key,last_error) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IBraveVpnWireguardManager_INTERFACE_DEFINED__ */



#ifndef __BraveVpnWireguardServiceLib_LIBRARY_DEFINED__
#define __BraveVpnWireguardServiceLib_LIBRARY_DEFINED__

/* library BraveVpnWireguardServiceLib */
/* [helpstring][version][uuid] */ 



EXTERN_C const IID LIBID_BraveVpnWireguardServiceLib;
#endif /* __BraveVpnWireguardServiceLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

unsigned long             __RPC_USER  BSTR_UserSize(     unsigned long *, unsigned long            , BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserMarshal(  unsigned long *, unsigned char *, BSTR * ); 
unsigned char * __RPC_USER  BSTR_UserUnmarshal(unsigned long *, unsigned char *, BSTR * ); 
void                      __RPC_USER  BSTR_UserFree(     unsigned long *, BSTR * ); 

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


