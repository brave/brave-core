/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_SERVICE_WIREGUARD_SERVICE_RUNNER_H_
#define BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_SERVICE_WIREGUARD_SERVICE_RUNNER_H_

#include <wrl/client.h>

#include "base/functional/callback.h"
#include "base/no_destructor.h"
#include "base/win/windows_types.h"
#include "components/version_info/channel.h"

namespace brave_vpn {

class WireguardServiceRunner {
 public:
  static WireguardServiceRunner* GetInstance();

  WireguardServiceRunner(const WireguardServiceRunner&) = delete;
  WireguardServiceRunner& operator=(const WireguardServiceRunner&) = delete;

  int RunAsService();

  void SignalExit();

 private:
  friend class base::NoDestructor<WireguardServiceRunner>;

  WireguardServiceRunner();
  ~WireguardServiceRunner();

  // Handles object registration, message loop, and unregistration. Returns
  // when all registered objects are released.
  HRESULT Run();

  // Creates an out-of-proc WRL Module.
  void CreateWRLModule();

  // Unregisters the Service COM class factory object.
  void UnregisterClassObject();

  // Registers the Service COM class factory object so other applications can
  // connect to it. Returns the registration status.
  HRESULT RegisterClassObject();

  // Runs the service on the service thread.
  void WireguardServiceRunnerImpl();

  // The control handler of the service.
  static void WINAPI ServiceControlHandler(DWORD control);

  // The main service entry point.
  static void WINAPI WireguardServiceRunnerEntry(DWORD argc, wchar_t* argv[]);

  // Calls ::SetServiceStatus().
  void SetServiceStatus(DWORD state);

  // Calls ::CoInitializeSecurity to allow all users to create COM objects
  // within the server.
  static HRESULT InitializeComSecurity();

  // The action routine to be executed.
  int (WireguardServiceRunner::*run_routine_)();

  // Identifier of registered class objects used for unregistration.
  DWORD cookies_[1] = {};
  SERVICE_STATUS_HANDLE service_status_handle_ = nullptr;
  SERVICE_STATUS service_status_;
  base::OnceClosure quit_;
  version_info::Channel channel_ = version_info::Channel::UNKNOWN;
};

}  // namespace brave_vpn

#endif  // BRAVE_BROWSER_BRAVE_VPN_WIN_BRAVE_VPN_WIREGUARD_SERVICE_SERVICE_WIREGUARD_SERVICE_RUNNER_H_
