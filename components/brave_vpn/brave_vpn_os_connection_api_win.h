/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_OS_CONNECTION_API_WIN_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_OS_CONNECTION_API_WIN_H_

#include <windows.h>
#include <string>

#include "base/memory/weak_ptr.h"
#include "base/no_destructor.h"
#include "base/win/object_watcher.h"
#include "brave/components/brave_vpn/brave_vpn_os_connection_api.h"

namespace brave_vpn {
namespace internal {
enum class CheckConnectionResult;
}  // namespace internal

class BraveVPNOSConnectionAPIWin : public BraveVPNOSConnectionAPI,
                                   public base::win::ObjectWatcher::Delegate {
 public:
  BraveVPNOSConnectionAPIWin(const BraveVPNOSConnectionAPIWin&) = delete;
  BraveVPNOSConnectionAPIWin& operator=(const BraveVPNOSConnectionAPIWin&) =
      delete;

 protected:
  friend class base::NoDestructor<BraveVPNOSConnectionAPIWin>;

  BraveVPNOSConnectionAPIWin();
  ~BraveVPNOSConnectionAPIWin() override;

 private:
  // BraveVPNOSConnectionAPI overrides:
  void CreateVPNConnection(const BraveVPNConnectionInfo& info) override;
  void UpdateVPNConnection(const BraveVPNConnectionInfo& info) override;
  void RemoveVPNConnection(const std::string& name) override;
  void Connect(const std::string& name) override;
  void Disconnect(const std::string& name) override;
  void CheckConnection(const std::string& name) override;

  // base::win::ObjectWatcher::Delegate overrides:
  void OnObjectSignaled(HANDLE object) override;

  void OnCreated(const std::string& name, bool success);
  void OnRemoved(const std::string& name, bool success);
  void OnCheckConnection(const std::string& name,
                         internal::CheckConnectionResult result);

  void StartVPNConnectionChangeMonitoring();

  HANDLE event_handle_ = nullptr;
  base::win::ObjectWatcher watcher_;
  base::WeakPtrFactory<BraveVPNOSConnectionAPIWin> weak_factory_{this};
};

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BRAVE_VPN_OS_CONNECTION_API_WIN_H_
