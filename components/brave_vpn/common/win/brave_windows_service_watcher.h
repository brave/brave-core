/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_COMMON_WIN_BRAVE_WINDOWS_SERVICE_WATCHER_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_COMMON_WIN_BRAVE_WINDOWS_SERVICE_WATCHER_H_

#include <windows.h>

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "base/synchronization/waitable_event.h"
#include "base/synchronization/waitable_event_watcher.h"
#include "brave/components/brave_vpn/common/win/scoped_sc_handle.h"

namespace brave {

class ServiceWatcher {
 public:
  ServiceWatcher();
  virtual ~ServiceWatcher();

  using StateChangedCallback = base::RepeatingCallback<void(int)>;

  bool Subscribe(const std::wstring& service_name,
                 int mask,
                 StateChangedCallback callback);
  bool IsWatching() const;

  void StartWatching();
  std::wstring GetServiceName() const;

 protected:
  void OnServiceSignaled(base::WaitableEvent* service_event);

 private:
  bool is_watching_ = false;
  int mask_ = 0;
  ScopedScHandle scm_;
  ScopedScHandle service_;
  SERVICE_NOTIFY service_notify_{0};
  StateChangedCallback callback_;
  std::wstring service_name_;
  base::WaitableEvent service_stopped_event_;
  std::unique_ptr<base::WaitableEventWatcher> service_watcher_;
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  base::WeakPtrFactory<ServiceWatcher> weak_ptr_factory_{this};
};

}  // namespace brave

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_COMMON_WIN_BRAVE_WINDOWS_SERVICE_WATCHER_H_
