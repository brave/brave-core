/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

// This macro is used in <wrl/module.h>. Since only the COM functionality is
// used here (while WinRT isn't being used), define this macro to optimize
// compilation of <wrl/module.h> for COM-only.

#ifndef __WRL_CLASSIC_COM_STRICT__
#define __WRL_CLASSIC_COM_STRICT__
#endif  // __WRL_CLASSIC_COM_STRICT__

#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/service/wireguard_service_runner.h"

#include <atlsecurity.h>
#include <sddl.h>
#include <wrl/module.h>
#include <type_traits>
#include <utility>

#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/run_loop.h"
#include "base/task/single_thread_task_executor.h"
#include "base/threading/thread_restrictions.h"
#include "base/win/scoped_com_initializer.h"
#include "brave/browser/brave_vpn/win/brave_vpn_wireguard_service/service/brave_wireguard_manager.h"
#include "brave/components/brave_vpn/common/wireguard/win/service_details.h"
#include "brave/components/brave_vpn/common/wireguard/win/wireguard_utils_win.h"
#include "chrome/common/channel_info.h"

namespace brave_vpn {

WireguardServiceRunner* WireguardServiceRunner::GetInstance() {
  static base::NoDestructor<WireguardServiceRunner> instance;
  return instance.get();
}

void WireguardServiceRunner::CreateWRLModule() {
  Microsoft::WRL::Module<Microsoft::WRL::OutOfProc>::Create(
      this, &WireguardServiceRunner::SignalExit);
}

// When _WireguardServiceRunner gets called, it initializes COM, and then calls
// Run(). Run() initializes security, then calls RegisterClassObject().
HRESULT WireguardServiceRunner::RegisterClassObject() {
  auto& module = Microsoft::WRL::Module<Microsoft::WRL::OutOfProc>::GetModule();

  // We hand-register a unique CLSID for each Chrome channel.
  Microsoft::WRL::ComPtr<IUnknown> factory;
  unsigned int flags = Microsoft::WRL::ModuleType::OutOfProc;

  HRESULT hr = Microsoft::WRL::Details::CreateClassFactory<
      Microsoft::WRL::SimpleClassFactory<BraveWireguardManager>>(
      &flags, nullptr, __uuidof(IClassFactory), &factory);
  if (FAILED(hr)) {
    VLOG(1) << "Factory creation failed; hr: " << hr;
    return hr;
  }

  Microsoft::WRL::ComPtr<IClassFactory> class_factory;
  hr = factory.As(&class_factory);
  if (FAILED(hr)) {
    VLOG(1) << "IClassFactory object creation failed; hr: " << hr;
    return hr;
  }

  // The pointer in this array is unowned. Do not release it.
  IClassFactory* class_factories[] = {class_factory.Get()};
  static_assert(std::extent<decltype(cookies_)>() == std::size(class_factories),
                "Arrays cookies_ and class_factories must be the same size.");

  IID class_ids[] = {GetBraveVpnWireguardServiceClsid(channel_)};

  DCHECK_EQ(std::size(cookies_), std::size(class_ids));
  static_assert(std::extent<decltype(cookies_)>() == std::size(class_ids),
                "Arrays cookies_ and class_ids must be the same size.");

  hr = module.RegisterCOMObject(nullptr, class_ids, class_factories, cookies_,
                                std::size(cookies_));
  if (FAILED(hr)) {
    VLOG(1) << "RegisterCOMObject failed; hr: " << std::hex << hr;
    return hr;
  }

  return hr;
}

void WireguardServiceRunner::UnregisterClassObject() {
  auto& module = Microsoft::WRL::Module<Microsoft::WRL::OutOfProc>::GetModule();
  const HRESULT hr =
      module.UnregisterCOMObject(nullptr, cookies_, std::size(cookies_));
  if (FAILED(hr)) {
    VLOG(1) << "UnregisterCOMObject failed; hr: " << hr;
  }
}

WireguardServiceRunner::WireguardServiceRunner()
    : service_status_handle_(nullptr),
      service_status_(),
      channel_(chrome::GetChannel()) {
  service_status_.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
  service_status_.dwCurrentState = SERVICE_STOPPED;
  service_status_.dwControlsAccepted = SERVICE_ACCEPT_STOP;
}

WireguardServiceRunner::~WireguardServiceRunner() = default;

int WireguardServiceRunner::RunAsService() {
  const std::wstring& service_name(
      brave_vpn::GetBraveVpnWireguardServiceName(channel_));
  const SERVICE_TABLE_ENTRY dispatch_table[] = {
      {const_cast<LPTSTR>(service_name.c_str()),
       &WireguardServiceRunner::WireguardServiceRunnerEntry},
      {nullptr, nullptr}};

  if (!::StartServiceCtrlDispatcher(dispatch_table)) {
    service_status_.dwWin32ExitCode = ::GetLastError();
    VLOG(1) << "Failed to connect to the service control manager:"
            << service_status_.dwWin32ExitCode;
  }

  return service_status_.dwWin32ExitCode;
}

void WireguardServiceRunner::WireguardServiceRunnerImpl() {
  service_status_handle_ = ::RegisterServiceCtrlHandler(
      brave_vpn::GetBraveVpnWireguardServiceName(channel_).c_str(),
      &WireguardServiceRunner::ServiceControlHandler);
  if (service_status_handle_ == nullptr) {
    VLOG(1) << "RegisterServiceCtrlHandler failed";
    return;
  }
  SetServiceStatus(SERVICE_RUNNING);
  service_status_.dwWin32ExitCode = ERROR_SUCCESS;
  service_status_.dwCheckPoint = 0;
  service_status_.dwWaitHint = 0;

  // Initialize COM for the current thread.
  base::win::ScopedCOMInitializer com_initializer(
      base::win::ScopedCOMInitializer::kMTA);
  if (!com_initializer.Succeeded()) {
    PLOG(ERROR) << "Failed to initialize COM";
    SetServiceStatus(SERVICE_STOPPED);
    return;
  }
  // When the Run function returns, the service has stopped.
  const HRESULT hr = Run();
  if (FAILED(hr)) {
    service_status_.dwWin32ExitCode = ERROR_SERVICE_SPECIFIC_ERROR;
    service_status_.dwServiceSpecificExitCode = hr;
  }

  SetServiceStatus(SERVICE_STOPPED);
}

// static
void WireguardServiceRunner::ServiceControlHandler(DWORD control) {
  WireguardServiceRunner* self = WireguardServiceRunner::GetInstance();
  switch (control) {
    case SERVICE_CONTROL_STOP:
      self->SignalExit();
      break;

    default:
      break;
  }
}

// static
void WINAPI
WireguardServiceRunner::WireguardServiceRunnerEntry(DWORD argc,
                                                    wchar_t* argv[]) {
  WireguardServiceRunner::GetInstance()->WireguardServiceRunnerImpl();
}

void WireguardServiceRunner::SetServiceStatus(DWORD state) {
  ::InterlockedExchange(&service_status_.dwCurrentState, state);
  ::SetServiceStatus(service_status_handle_, &service_status_);
}

// static
HRESULT WireguardServiceRunner::InitializeComSecurity() {
  CDacl dacl;
  constexpr auto com_rights_execute_local =
      COM_RIGHTS_EXECUTE | COM_RIGHTS_EXECUTE_LOCAL;
  if (!dacl.AddAllowedAce(Sids::System(), com_rights_execute_local) ||
      !dacl.AddAllowedAce(Sids::Admins(), com_rights_execute_local) ||
      !dacl.AddAllowedAce(Sids::Interactive(), com_rights_execute_local)) {
    return E_ACCESSDENIED;
  }

  CSecurityDesc sd;
  sd.SetDacl(dacl);
  sd.MakeAbsolute();
  sd.SetOwner(Sids::Admins());
  sd.SetGroup(Sids::Admins());

  // These are the flags being set:
  // EOAC_DYNAMIC_CLOAKING: DCOM uses the thread token (if present) when
  //   determining the client's identity. Useful when impersonating another
  //   user.
  // EOAC_SECURE_REFS: Authenticates distributed reference count calls to
  //   prevent malicious users from releasing objects that are still being used.
  // EOAC_DISABLE_AAA: Causes any activation where a server process would be
  //   launched under the caller's identity (activate-as-activator) to fail with
  //   E_ACCESSDENIED.
  // EOAC_NO_CUSTOM_MARSHAL: reduces the chances of executing arbitrary DLLs
  //   because it allows the marshaling of only CLSIDs that are implemented in
  //   Ole32.dll, ComAdmin.dll, ComSvcs.dll, or Es.dll, or that implement the
  //   CATID_MARSHALER category ID.
  // RPC_C_AUTHN_LEVEL_PKT_PRIVACY: prevents replay attacks, verifies that none
  //   of the data transferred between the client and server has been modified,
  //   ensures that the data transferred can only be seen unencrypted by the
  //   client and the server.
  return ::CoInitializeSecurity(
      const_cast<SECURITY_DESCRIPTOR*>(sd.GetPSECURITY_DESCRIPTOR()), -1,
      nullptr, nullptr, RPC_C_AUTHN_LEVEL_PKT_PRIVACY, RPC_C_IMP_LEVEL_IDENTIFY,
      nullptr,
      EOAC_DYNAMIC_CLOAKING | EOAC_DISABLE_AAA | EOAC_SECURE_REFS |
          EOAC_NO_CUSTOM_MARSHAL,
      nullptr);
}

HRESULT WireguardServiceRunner::Run() {
  HRESULT hr = InitializeComSecurity();
  if (FAILED(hr)) {
    return hr;
  }

  CreateWRLModule();
  hr = RegisterClassObject();
  if (SUCCEEDED(hr)) {
    base::SingleThreadTaskExecutor service_task_executor(
        base::MessagePumpType::UI);
    base::RunLoop loop;
    quit_ = loop.QuitClosure();
    loop.Run();
    UnregisterClassObject();
  }

  return S_OK;
}

void WireguardServiceRunner::SignalExit() {
  std::move(quit_).Run();
}

}  // namespace brave_vpn
