/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_PIN_IPFS_LOCAL_PIN_SERVICE_H_
#define BRAVE_COMPONENTS_IPFS_PIN_IPFS_LOCAL_PIN_SERVICE_H_

#include "brave/components/ipfs/ipfs_service.h"
#include "brave/components/ipfs/pin/ipfs_base_pin_service.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

#include <vector>

using ipfs::IpfsService;

namespace ipfs {

using AddPinCallback = base::OnceCallback<void(bool)>;
using RemovePinCallback = base::OnceCallback<void(bool)>;
using ValidatePinsCallback = base::OnceCallback<void(bool)>;

class AddLocalPinJob : public IpfsCIDListJob {
 public:
  AddLocalPinJob(PrefService* prefs_service,
                 IpfsService* ipfs_service,
                 const std::string& prefix,
                 const std::vector<std::string>& cids,
                 JobFinishedCallback callback);
  ~AddLocalPinJob() override;

 protected:
  void DoWork(const std::string& cid) override;

 private:
  void OnAddPinResult(bool status, absl::optional<AddPinResult> result);

  PrefService* prefs_service_;
  IpfsService* ipfs_service_;
  std::string prefix_;
};

class RemoveLocalPinJob : public IpfsCIDListJob {
 public:
  RemoveLocalPinJob(PrefService* prefs_service,
                    IpfsService* ipfs_service,
                    const std::string& prefix,
                    const std::vector<std::string>& cids,
                    JobFinishedCallback callback);
  ~RemoveLocalPinJob() override;

 protected:
  void DoWork(const std::string& cid) override;

 private:
  void OnRemovePinResult(bool status, absl::optional<RemovePinResult> result);

  PrefService* prefs_service_;
  IpfsService* ipfs_service_;
  std::string prefix_;
};

class VerifyLocalPinJob : public IpfsCIDListJob {
 public:
  VerifyLocalPinJob(PrefService* prefs_service,
                    IpfsService* ipfs_service,
                    const std::string& prefix,
                    const std::vector<std::string>& cids,
                    JobFinishedCallback callback);
  ~VerifyLocalPinJob() override;

 protected:
  void DoWork(const std::string& cid) override;

 private:
  void OnGetPinsResult(bool status, absl::optional<GetPinsResult> result);

  PrefService* prefs_service_;
  IpfsService* ipfs_service_;
  std::string prefix_;
};

class IpfsLocalPinService : public KeyedService {
 public:
  IpfsLocalPinService(PrefService* prefs_service, IpfsService* ipfs_service);
  ~IpfsLocalPinService() override;

  void AddPins(const std::string& prefix,
               const std::vector<std::string>& cids,
               AddPinCallback callback);

  void RemovePins(const std::string& prefix,
                  const std::vector<std::string>& cids,
                  RemovePinCallback callback);

  void ValidatePins(const std::string& prefix,
                    const std::vector<std::string>& cids,
                    ValidatePinsCallback callback);

 private:
  void OnGetPinsResult(const std::string& prefix,
                       const std::vector<std::string>& cids,
                       ValidatePinsCallback callback,
                       bool status,
                       absl::optional<GetPinsResult> result);
  void OnRemovePinsFinished(RemovePinCallback callback, bool status);
  void OnAddJobFinished(AddPinCallback callback, bool status);
  void OnVerifyJobFinised(ValidatePinsCallback callback, bool status);

  std::unique_ptr<IpfsBasePinService> ipfs_base_pin_service_;
  PrefService* prefs_service_;
  IpfsService* ipfs_service_;
};

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_PIN_IPFS_LOCAL_PIN_SERVICE_H_
