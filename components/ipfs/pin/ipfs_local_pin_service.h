/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_IPFS_PIN_IPFS_LOCAL_PIN_SERVICE_H_
#define BRAVE_COMPONENTS_IPFS_PIN_IPFS_LOCAL_PIN_SERVICE_H_

#include <memory>
#include <string>
#include <vector>

#include "brave/components/ipfs/ipfs_service.h"
#include "brave/components/ipfs/pin/ipfs_base_pin_service.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

using ipfs::IpfsService;

namespace ipfs {

using AddPinCallback = base::OnceCallback<void(bool)>;
using RemovePinCallback = base::OnceCallback<void(bool)>;
using ValidatePinsCallback = base::OnceCallback<void(absl::optional<bool>)>;
using GcCallback = base::OnceCallback<void(bool)>;

class AddLocalPinJob : public IpfsBaseJob {
 public:
  AddLocalPinJob(PrefService* prefs_service,
                 IpfsService* ipfs_service,
                 const std::string& prefix,
                 const std::vector<std::string>& cids,
                 AddPinCallback callback);
  ~AddLocalPinJob() override;

  void Start() override;

 private:
  void OnAddPinResult(bool status, absl::optional<AddPinResult> result);

  PrefService* prefs_service_;
  IpfsService* ipfs_service_;
  std::string prefix_;
  std::vector<std::string> cids_;
  AddPinCallback callback_;
};

class RemoveLocalPinJob : public IpfsBaseJob {
 public:
  RemoveLocalPinJob(PrefService* prefs_service,
                    const std::string& prefix,
                    RemovePinCallback callback);
  ~RemoveLocalPinJob() override;

  void Start() override;

 private:
  PrefService* prefs_service_;
  std::string prefix_;
  RemovePinCallback callback_;
};

class VerifyLocalPinJob : public IpfsBaseJob {
 public:
  VerifyLocalPinJob(PrefService* prefs_service,
                    IpfsService* ipfs_service,
                    const std::string& prefix,
                    const std::vector<std::string>& cids,
                    ValidatePinsCallback callback);
  ~VerifyLocalPinJob() override;

  void Start() override;

 private:
  void OnGetPinsResult(bool status, absl::optional<GetPinsResult> result);

  PrefService* prefs_service_;
  IpfsService* ipfs_service_;
  std::string prefix_;
  std::vector<std::string> cids_;
  ValidatePinsCallback callback_;
};

class GcJob : public IpfsBaseJob {
 public:
  GcJob(PrefService* prefs_service,
        IpfsService* ipfs_service,
        GcCallback callback);
  ~GcJob() override;

  void Start() override;

 private:
  void OnGetPinsResult(bool status, absl::optional<GetPinsResult> result);
  void OnPinsRemovedResult(bool status, absl::optional<RemovePinResult> result);

  PrefService* prefs_service_;
  IpfsService* ipfs_service_;
  GcCallback callback_;
};

class IpfsLocalPinService : public KeyedService {
 public:
  IpfsLocalPinService(PrefService* prefs_service, IpfsService* ipfs_service);
  // For testing
  IpfsLocalPinService();
  ~IpfsLocalPinService() override;

  virtual void AddPins(const std::string& prefix,
                       const std::vector<std::string>& cids,
                       AddPinCallback callback);
  virtual void RemovePins(const std::string& prefix,
                          RemovePinCallback callback);
  virtual void ValidatePins(const std::string& prefix,
                            const std::vector<std::string>& cids,
                            ValidatePinsCallback callback);

  void SetIpfsBasePinServiceForTesting(std::unique_ptr<IpfsBasePinService>);

 private:
  void AddGcTask();
  void OnRemovePinsFinished(RemovePinCallback callback, bool status);
  void OnAddJobFinished(AddPinCallback callback, bool status);
  void OnValidateJobFinished(ValidatePinsCallback callback,
                             absl::optional<bool> status);
  void OnGcFinishedCallback(bool status);

  bool gc_task_posted_ = false;
  std::unique_ptr<IpfsBasePinService> ipfs_base_pin_service_;
  PrefService* prefs_service_;
  IpfsService* ipfs_service_;
};

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_PIN_IPFS_LOCAL_PIN_SERVICE_H_
