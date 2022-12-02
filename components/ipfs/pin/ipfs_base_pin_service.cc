/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/ipfs/pin/ipfs_base_pin_service.h"

#include "brave/components/ipfs/ipfs_utils.h"
#include "brave/components/ipfs/pref_names.h"

namespace ipfs {

IpfsBaseJob::IpfsBaseJob() {}

IpfsBaseJob::~IpfsBaseJob() {}

IpfsBasePinService::IpfsBasePinService(PrefService* pref_service,
                                       IpfsService* ipfs_service)
    : pref_service_(pref_service), ipfs_service_(ipfs_service) {
  ipfs_service_->AddObserver(this);
  pref_change_registrar_.Init(pref_service_);
  pref_change_registrar_.Add(
      kIPFSResolveMethod,
      base::BindRepeating(&IpfsBasePinService::MaybeStartDaemon,
                          base::Unretained(this)));
}

IpfsBasePinService::IpfsBasePinService() {}

IpfsBasePinService::~IpfsBasePinService() {}

// For unit tests
void IpfsBasePinService::RemovePrefListenersForTests() {
  pref_change_registrar_.RemoveAll();
}

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

  if (!ipfs::IsLocalGatewayConfigured(pref_service_)) {
    return;
  }

  ipfs_service_->StartDaemonAndLaunch(base::BindOnce(
      &IpfsBasePinService::OnDaemonStarted, base::Unretained(this)));
}

void IpfsBasePinService::OnDaemonStarted() {
  ipfs_service_->GetConnectedPeers(base::NullCallback(), 2);
}

}  // namespace ipfs
