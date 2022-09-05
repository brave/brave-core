/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_PIN_IPFS_REMOTE_PIN_SERVICE_H_
#define BRAVE_COMPONENTS_IPFS_PIN_IPFS_REMOTE_PIN_SERVICE_H_

#include "brave/components/ipfs/ipfs_service.h"
#include "brave/components/ipfs/pin/ipfs_pin_rpc_types.h"
#include "brave/components/ipfs/pin/ipfs_pin_service_add_job.h"

#include <vector>

namespace ipfs {

using AddPinCallback = base::OnceCallback<void(bool)>;
using RemovePinsCallback = base::OnceCallback<void(bool)>;
using GetPinStatusCallback = base::OnceCallback<void(bool)>;

using AddRemotePinServiceCallback = base::OnceCallback<void(bool)>;
using GetRemotePinServicesCallback =
    base::OnceCallback<void(absl::optional<GetRemotePinServicesResult>)>;
using RemoveRemotePinServiceCallback = base::OnceCallback<void(bool)>;

class IPFSRemotePinService : public KeyedService {
 public:
  explicit IPFSRemotePinService(IpfsService* service);
  ~IPFSRemotePinService() override;

  void AddRemotePinService(const std::string& name,
                           const std::string& endpoint,
                           const std::string& key,
                           AddRemotePinServiceCallback callback);

  void RemoveRemotePinService(const std::string& name,
                              RemoveRemotePinServiceCallback callback);

  void GetRemotePinServices(GetRemotePinServicesCallback callback);

  void AddPins(const std::string& service_name,
               const std::string& prefix,
               const std::vector<std::string>& cids,
               AddPinCallback callback);

  void RemovePins(const std::string& service_name,
                  const std::string& prefix,
                  const std::vector<std::string>& cids,
                  RemovePinsCallback callback);

  void GetPinStatus(const std::string& service_name,
                    const std::string& prefix,
                    const std::vector<std::string>& cids,
                    GetPinStatusCallback callback);

 private:
  bool AwaitUntilDaemonStart();
  void OnDaemonStarted();

  void OnAddJobFinished(AddPinCallback callback, bool result);

  // void OnGetPinsResult(GetPinsCallback callback,
  //                      bool result,
  //                      const std::vector<std::string>& items);

  void AddJob(std::unique_ptr<IpfsBaseJob> job);
  void DoNextJob();
  void OnJobDone(bool result);

  IpfsService* ipfs_service_;
  std::unique_ptr<IpfsBaseJob> current_job_;
  std::queue<std::unique_ptr<IpfsBaseJob>> jobs_;
};

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_PIN_IPFS_REMOTE_PIN_SERVICE_H_
