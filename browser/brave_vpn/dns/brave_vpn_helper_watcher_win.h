/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_VPN_DNS_BRAVE_VPN_HELPER_WATCHER_WIN_H_
#define BRAVE_BROWSER_BRAVE_VPN_DNS_BRAVE_VPN_HELPER_WATCHER_WIN_H_

#include <windows.h>
#include <string>

#include "base/memory/weak_ptr.h"
#include "base/synchronization/waitable_event.h"
#include "base/synchronization/waitable_event_watcher.h"
#include "brave/components/brave_vpn/browser/connection/common/win/scoped_sc_handle.h"

class ServiceWatcher {
 public:
  ServiceWatcher();
  virtual ~ServiceWatcher();

  bool Subscribe(const std::wstring& service_name,
                 int state,
                 base::OnceClosure callback);

 protected:
  void OnServiceSignaled(base::OnceClosure callback,
                         base::WaitableEvent* service_event);

 private:
  ScopedScHandle scm_;
  ScopedScHandle service_;
  SERVICE_NOTIFY service_notify_{0};
  base::WaitableEvent service_stopped_event_;
  base::WaitableEventWatcher service_watcher_;
  scoped_refptr<base::SequencedTaskRunner> task_runner_;
  base::WeakPtrFactory<ServiceWatcher> weak_ptr_factory_{this};
};

#endif  // BRAVE_BROWSER_BRAVE_VPN_DNS_BRAVE_VPN_HELPER_WATCHER_WIN_H_
