/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_PIN_IPFS_BASE_PIN_SERVICE_H_
#define BRAVE_COMPONENTS_IPFS_PIN_IPFS_BASE_PIN_SERVICE_H_

#include "brave/components/ipfs/ipfs_service.h"

#include <vector>

namespace ipfs {

class IpfsBaseJob {
 public:
  IpfsBaseJob();
  virtual ~IpfsBaseJob();
  virtual void Start() = 0;
  virtual void Cancel() = 0;
};

using JobFinishedCallback = base::OnceCallback<void(bool)>;

class IpfsCIDListJob : public IpfsBaseJob {
 public:
  IpfsCIDListJob(const std::vector<std::string> items,
                 JobFinishedCallback callback);

  ~IpfsCIDListJob() override;

  void Start() override;
  void Cancel() override;

 protected:
  virtual void DoWork(const std::string& cid) = 0;
  void NotifyJobFinished(bool result, int error_code);
  void Fail();
  void Continue();

 private:
  size_t counter_ = 0;
  std::vector<std::string> items_;
  bool canceled_ = false;

  JobFinishedCallback callback_;
};

class IpfsBasePinService {
 public:
  explicit IpfsBasePinService(IpfsService* service);
  ~IpfsBasePinService();

  void AddJob(std::unique_ptr<IpfsBaseJob> job);
  void OnJobDone(bool result);

 private:
  bool AwaitUntilDaemonStart();
  void OnDaemonStarted(bool result);

  void DoNextJob();

  IpfsService* ipfs_service_;
  std::unique_ptr<IpfsBaseJob> current_job_;
  std::queue<std::unique_ptr<IpfsBaseJob>> jobs_;
};

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_PIN_IPFS_BASE_PIN_SERVICE_H_
