/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/common/win/brave_windows_service_watcher.h"

#include <utility>

#include "base/logging.h"
#include "base/task/task_traits.h"
#include "base/task/thread_pool.h"

namespace brave {

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
                  int mask,
                  SERVICE_NOTIFY* service_notify) {
  auto result = NotifyServiceStatusChange(service, mask, service_notify);
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
           base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN})) {}

ServiceWatcher::~ServiceWatcher() = default;

std::wstring ServiceWatcher::GetServiceName() const {
  return service_name_;
}

bool ServiceWatcher::Subscribe(const std::wstring& service_name,
                               int mask,
                               StateChangedCallback callback) {
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
  service_name_ = service_name;
  mask_ = mask;
  callback_ = std::move(callback);
  service_notify_ = {
      .dwVersion = SERVICE_NOTIFY_STATUS_CHANGE,
      .pfnNotifyCallback = (PFN_SC_NOTIFY_CALLBACK)&OnServiceStoppedCallback,
      .pContext = service_stopped_event_.handle()};

  StartWatching();
  return true;
}

void ServiceWatcher::StartWatching() {
  DCHECK(service_.IsValid());
  DCHECK(!service_name_.empty());

  if (service_watcher_) {
    service_watcher_->StopWatching();
  }
  service_stopped_event_.Reset();
  service_watcher_.reset(new base::WaitableEventWatcher());
  task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&WaitForEvent, service_stopped_event_.handle(),
                                service_.get(), mask_, &service_notify_));
  service_watcher_->StartWatching(
      &service_stopped_event_,
      base::BindOnce(&ServiceWatcher::OnServiceSignaled,
                     weak_ptr_factory_.GetWeakPtr()),
      task_runner_);
  is_watching_ = true;
}

bool ServiceWatcher::IsWatching() const {
  return is_watching_;
}

void ServiceWatcher::OnServiceSignaled(base::WaitableEvent* service_event) {
  is_watching_ = false;
  if (!callback_) {
    return;
  }
  callback_.Run(mask_);
}

}  // namespace brave
