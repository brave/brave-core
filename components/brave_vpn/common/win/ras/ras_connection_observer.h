/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_COMMON_WIN_RAS_RAS_CONNECTION_OBSERVER_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_COMMON_WIN_RAS_RAS_CONNECTION_OBSERVER_H_

#include "base/win/object_watcher.h"
#include "base/win/scoped_handle.h"
#include "base/win/windows_types.h"

namespace brave_vpn {

namespace ras {
class RasConnectionObserver : public base::win::ObjectWatcher::Delegate {
 public:
  RasConnectionObserver();

  RasConnectionObserver(const RasConnectionObserver&) = delete;
  RasConnectionObserver& operator=(const RasConnectionObserver&) = delete;

  ~RasConnectionObserver() override;

  void StartRasConnectionChangeMonitoring();
  void StopRasConnectionChangeMonitoring();
  bool IsRasConnectionObserverActive() const;

  // base::win::ObjectWatcher::Delegate overides:
  void OnObjectSignaled(HANDLE object) override;

  virtual void OnRasConnectionStateChanged() = 0;

 private:
  base::win::ScopedHandle event_handle_for_connected_disconnected_;
  base::win::ObjectWatcher connected_disconnected_event_watcher_;
};

}  // namespace ras
}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_COMMON_WIN_RAS_RAS_CONNECTION_OBSERVER_H_
