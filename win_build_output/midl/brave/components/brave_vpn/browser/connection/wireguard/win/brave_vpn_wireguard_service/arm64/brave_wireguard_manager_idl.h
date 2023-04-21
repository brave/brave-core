

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.xx.xxxx */
/* at a redacted point in time
 */
/* Compiler settings for ../../brave/components/brave_vpn/browser/connection/wireguard/win/brave_vpn_wireguard_service/brave_wireguard_manager_idl.idl:
    Oicf, W1, Zp8, env=Win64 (32b run), target_arch=ARM64 8.01.0628 
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

#ifndef __IBraveWireguardManager_FWD_DEFINED__
#define __IBraveWireguardManager_FWD_DEFINED__
typedef interface IBraveWireguardManager IBraveWireguardManager;

#endif 	/* __IBraveWireguardManager_FWD_DEFINED__ */


#ifndef __IBraveWireguardManager_FWD_DEFINED__
#define __IBraveWireguardManager_FWD_DEFINED__
typedef interface IBraveWireguardManager IBraveWireguardManager;

#endif 	/* __IBraveWireguardManager_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __IBraveWireguardManager_INTERFACE_DEFINED__
#define __IBraveWireguardManager_INTERFACE_DEFINED__

/* interface IBraveWireguardManager */
/* [unique][helpstring][uuid][oleautomation][object] */ 


EXTERN_C const IID IID_IBraveWireguardManager;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("053057AB-CF06-4E6C-BBAD-F8DA6436D933")
    IBraveWireguardManager : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE EnableVpn( 
            /* [string][in] */ const WCHAR *config,
            /* [out] */ DWORD *last_error) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE DisableVpn( 
            /* [out] */ DWORD *last_error) = 0;
        
        virtual HRESULT STDMETHODCALLTYPE GenerateKeypair( 
            /* [out] */ BSTR *public_key,
            /* [out] */ BSTR *private_key,
            /* [out] */ DWORD *last_error) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IBraveWireguardManagerVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBraveWireguardManager * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBraveWireguardManager * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBraveWireguardManager * This);
        
        DECLSPEC_XFGVIRT(IBraveWireguardManager, EnableVpn)
        HRESULT ( STDMETHODCALLTYPE *EnableVpn )( 
            IBraveWireguardManager * This,
            /* [string][in] */ const WCHAR *config,
            /* [out] */ DWORD *last_error);
        
        DECLSPEC_XFGVIRT(IBraveWireguardManager, DisableVpn)
        HRESULT ( STDMETHODCALLTYPE *DisableVpn )( 
            IBraveWireguardManager * This,
            /* [out] */ DWORD *last_error);
        
        DECLSPEC_XFGVIRT(IBraveWireguardManager, GenerateKeypair)
        HRESULT ( STDMETHODCALLTYPE *GenerateKeypair )( 
            IBraveWireguardManager * This,
            /* [out] */ BSTR *public_key,
            /* [out] */ BSTR *private_key,
            /* [out] */ DWORD *last_error);
        
        END_INTERFACE
    } IBraveWireguardManagerVtbl;

    interface IBraveWireguardManager
    {
        CONST_VTBL struct IBraveWireguardManagerVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBraveWireguardManager_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IBraveWireguardManager_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IBraveWireguardManager_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IBraveWireguardManager_EnableVpn(This,config,last_error)	\
    ( (This)->lpVtbl -> EnableVpn(This,config,last_error) ) 

#define IBraveWireguardManager_DisableVpn(This,last_error)	\
    ( (This)->lpVtbl -> DisableVpn(This,last_error) ) 

#define IBraveWireguardManager_GenerateKeypair(This,public_key,private_key,last_error)	\
    ( (This)->lpVtbl -> GenerateKeypair(This,public_key,private_key,last_error) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IBraveWireguardManager_INTERFACE_DEFINED__ */



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


