// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ipfs/pin/ipfs_base_pin_service.h"

#include "brave/components/ipfs/ipfs_utils.h"
#include "brave/components/ipfs/pref_names.h"

namespace ipfs {

IpfsBaseJob::IpfsBaseJob() = default;

IpfsBaseJob::~IpfsBaseJob() = default;

IpfsBasePinService::IpfsBasePinService(IpfsService* ipfs_service)
    : ipfs_service_(ipfs_service) {
  ipfs_service_->AddObserver(this);
}

IpfsBasePinService::IpfsBasePinService() = default;

IpfsBasePinService::~IpfsBasePinService() = default;

void IpfsBasePinService::OnIpfsShutdown() {
  daemon_ready_ = false;
}

void IpfsBasePinService::OnGetConnectedPeers(
    bool success,
    const std::vector<std::string>& peers) {
  if (success) {
    daemon_ready_ = true;
    DoNextJob();
  }
}

void IpfsBasePinService::AddJob(std::unique_ptr<IpfsBaseJob> job) {
  jobs_.push(std::move(job));
  if (!current_job_) {
    DoNextJob();
  }
}

void IpfsBasePinService::DoNextJob() {
  if (jobs_.empty()) {
    return;
  }

  if (!IsDaemonReady()) {
    MaybeStartDaemon();
    return;
  }

  DCHECK(!current_job_);

  current_job_ = std::move(jobs_.front());
  jobs_.pop();

  current_job_->Start();
}

void IpfsBasePinService::OnJobDone(bool result) {
  current_job_.reset();
  DoNextJob();
}

bool IpfsBasePinService::IsDaemonReady() {
  return daemon_ready_;
}

void IpfsBasePinService::MaybeStartDaemon() {
  if (daemon_ready_) {
    return;
  }

  ipfs_service_->StartDaemonAndLaunch(base::BindOnce(
      &IpfsBasePinService::OnDaemonStarted, weak_ptr_factory_.GetWeakPtr()));
}

void IpfsBasePinService::OnDaemonStarted() {
  // Ensure that daemon service is fully initialized
  ipfs_service_->GetConnectedPeers(base::NullCallback(), 2);
}

}  // namespace ipfs
