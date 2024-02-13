// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/components/ipfs/pin/ipfs_base_pin_service.h"

#include <optional>

#include "brave/components/ipfs/ipfs_utils.h"
#include "brave/components/ipfs/pref_names.h"

namespace ipfs {

IpfsBaseJob::IpfsBaseJob() = default;

IpfsBaseJob::~IpfsBaseJob() = default;

void IpfsBaseJob::Cancel() {
  is_canceled_ = true;
}

IpfsBasePinService::IpfsBasePinService(IpfsService* ipfs_service)
    : ipfs_service_(ipfs_service) {
  ipfs_service_->AddObserver(this);
}

IpfsBasePinService::IpfsBasePinService() = default;

IpfsBasePinService::~IpfsBasePinService() {
  if (ipfs_service_) {
    ipfs_service_->RemoveObserver(this);
  }
}

void IpfsBasePinService::OnIpfsShutdown() {
  daemon_ready_ = false;
  if (current_job_) {
    current_job_->Cancel();
    current_job_.reset();
  }
}

void IpfsBasePinService::OnGetConnectedPeersResult(
    size_t attempt,
    bool success,
    const std::vector<std::string>& peers) {
  if (daemon_ready_) {
    return;
  }
  if (success) {
    daemon_ready_ = true;
    DoNextJob();
  } else {
    PostGetConnectedPeers(attempt++);
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

  ipfs_service_->StartDaemonAndLaunch(
      base::BindOnce(&IpfsBasePinService::PostGetConnectedPeers,
                     weak_ptr_factory_.GetWeakPtr(), 1));
}

void IpfsBasePinService::PostGetConnectedPeers(size_t attempt) {
  if (daemon_ready_) {
    return;
  }

  if (!ipfs_service_->IsDaemonLaunched()) {
    return;
  }

  if (jobs_.empty()) {
    return;
  }

  if (attempt > 5) {
    return;
  }

  // Ensure that daemon service is fully initialized
  base::SequencedTaskRunner::GetCurrentDefault()->PostDelayedTask(
      FROM_HERE,
      base::BindOnce(&IpfsBasePinService::GetConnectedPeers,
                     weak_ptr_factory_.GetWeakPtr(), attempt),
      base::Seconds(30 * attempt));
}

void IpfsBasePinService::GetConnectedPeers(size_t attempt) {
  ipfs_service_->GetConnectedPeers(
      base::BindOnce(&IpfsBasePinService::OnGetConnectedPeersResult,
                     weak_ptr_factory_.GetWeakPtr(), attempt),
      std::nullopt);
}

bool IpfsBasePinService::HasJobs() {
  return current_job_ || !jobs_.empty();
}

}  // namespace ipfs
