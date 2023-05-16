/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_vpn/dns/brave_vpn_helper_watcher_win.h"

#include <utility>

#include "base/logging.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"
#include "brave/components/brave_vpn/browser/connection/common/win/scoped_sc_handle.h"
#include "content/public/browser/browser_task_traits.h"
#include "content/public/browser/browser_thread.h"

namespace {

VOID CALLBACK OnServiceStoppedCallback(PVOID pParameter) {
  SERVICE_NOTIFY* service_notify =
      reinterpret_cast<SERVICE_NOTIFY*>(pParameter);
  if (!service_notify || !service_notify->pContext) {
    return;
  }
  SetEvent(service_notify->pContext);
}

void WaitForEvent(HANDLE event,
                  SC_HANDLE service,
                  SERVICE_NOTIFY* service_notify) {
  auto result = NotifyServiceStatusChange(service, SERVICE_NOTIFY_STOPPED,
                                          service_notify);
  if (result != ERROR_SUCCESS) {
    VLOG(1) << "Unable to subscribe for service notifications:"
            << logging::SystemErrorCodeToString(result);
    // If we're unable to subscribe to status changes for this service,
    // the service may be in a bad state.
    // We can immediately signal to trigger the DoH fallback behavior.
    SetEvent(event);
    return;
  }

  ::WaitForSingleObjectEx(event, INFINITE, TRUE);
}

}  // namespace

ServiceWatcher::ServiceWatcher()
    : task_runner_(base::ThreadPool::CreateSequencedTaskRunner(
          {base::MayBlock(), base::TaskPriority::USER_VISIBLE,
           base::TaskShutdownBehavior::SKIP_ON_SHUTDOWN})) {}
ServiceWatcher::~ServiceWatcher() = default;

void ServiceWatcher::OnServiceSignaled(base::OnceClosure callback,
                                       base::WaitableEvent* service_event) {
  if (callback) {
    std::move(callback).Run();
  }
}

bool ServiceWatcher::Subscribe(const std::wstring& service_name,
                               int state,
                               base::OnceClosure callback) {
  scm_.Set(::OpenSCManager(
      nullptr, nullptr, SERVICE_QUERY_STATUS | SC_MANAGER_ENUMERATE_SERVICE));
  if (!scm_.IsValid()) {
    return false;
  }
  service_.Set(
      ::OpenService(scm_.Get(), service_name.c_str(), SERVICE_QUERY_STATUS));

  if (!service_.IsValid()) {
    return false;
  }

  service_notify_ = {
      .dwVersion = SERVICE_NOTIFY_STATUS_CHANGE,
      .pfnNotifyCallback = (PFN_SC_NOTIFY_CALLBACK)&OnServiceStoppedCallback,
      .pContext = service_stopped_event_.handle()};

  task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&WaitForEvent, service_stopped_event_.handle(),
                                service_.get(), &service_notify_));

  service_watcher_.StartWatching(
      &service_stopped_event_,
      base::BindOnce(&ServiceWatcher::OnServiceSignaled,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)),
      task_runner_);

  return true;
}
