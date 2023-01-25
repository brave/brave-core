// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

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

/**
 * Pins provided cids and writes record to kIPFSPinnedCids:
 * {
 *   // List of all pinned CIDs
 *   "Qme1": [
 *     // List of tokens that contain this CID
 *     "nft.local.60.0x1.0xbc4ca0eda7647a8ab7c2061c2e118a18a936f13d.0x1"
 *     "nft.local.60.0x1.0xbc4ca0eda7647a8ab7c2061c2e118a18a936f13d.0x2"
 *   ],
 *   "Qme2": [
 *     "nft.local.60.0x1.0xbc4ca0eda7647a8ab7c2061c2e118a18a936f13d.0x1"
 *   ],
 *   "Qme3": [
 *     "nft.local.60.0x1.0xbc4ca0eda7647a8ab7c2061c2e118a18a936f13d.0x2"
 *   ]
 * }
 */
class AddLocalPinJob : public IpfsBaseJob {
 public:
  AddLocalPinJob(PrefService* prefs_service,
                 IpfsService* ipfs_service,
                 const std::string& key,
                 const std::vector<std::string>& cids,
                 AddPinCallback callback);
  ~AddLocalPinJob() override;

  void Start() override;

 private:
  void OnAddPinResult(absl::optional<AddPinResult> result);

  raw_ptr<PrefService> prefs_service_;
  raw_ptr<IpfsService> ipfs_service_;
  std::string key_;
  std::vector<std::string> cids_;
  AddPinCallback callback_;
  base::WeakPtrFactory<AddLocalPinJob> weak_ptr_factory_{this};
};

// Removes records related to the key and launches GC task.
class RemoveLocalPinJob : public IpfsBaseJob {
 public:
  RemoveLocalPinJob(PrefService* prefs_service,
                    const std::string& key,
                    RemovePinCallback callback);
  ~RemoveLocalPinJob() override;

  void Start() override;

 private:
  raw_ptr<PrefService> prefs_service_;
  std::string key_;
  RemovePinCallback callback_;
  base::WeakPtrFactory<RemoveLocalPinJob> weak_ptr_factory_{this};
};

// Verifies that cids are actually pinned
class VerifyLocalPinJob : public IpfsBaseJob {
 public:
  VerifyLocalPinJob(PrefService* prefs_service,
                    IpfsService* ipfs_service,
                    const std::string& key,
                    const std::vector<std::string>& cids,
                    ValidatePinsCallback callback);
  ~VerifyLocalPinJob() override;

  void Start() override;

 private:
  void OnGetPinsResult(absl::optional<GetPinsResult> result);

  raw_ptr<PrefService> prefs_service_;
  raw_ptr<IpfsService> ipfs_service_;
  std::string key_;
  std::vector<std::string> cids_;
  ValidatePinsCallback callback_;
  base::WeakPtrFactory<VerifyLocalPinJob> weak_ptr_factory_{this};
};

// Unpins cids that don't have kIPFSPinnedCids record
class GcJob : public IpfsBaseJob {
 public:
  GcJob(PrefService* prefs_service,
        IpfsService* ipfs_service,
        GcCallback callback);
  ~GcJob() override;

  void Start() override;

 private:
  void OnGetPinsResult(absl::optional<GetPinsResult> result);
  void OnPinsRemovedResult(absl::optional<RemovePinResult> result);

  raw_ptr<PrefService> prefs_service_;
  raw_ptr<IpfsService> ipfs_service_;
  GcCallback callback_;
  base::WeakPtrFactory<GcJob> weak_ptr_factory_{this};
};

class IpfsLocalPinService : public KeyedService {
 public:
  IpfsLocalPinService(PrefService* prefs_service, IpfsService* ipfs_service);
  // For testing
  IpfsLocalPinService();
  ~IpfsLocalPinService() override;

  // Pins provided cids and stores related record in the prefs.
  virtual void AddPins(const std::string& key,
                       const std::vector<std::string>& cids,
                       AddPinCallback callback);
  // Unpins all cids related to the key.
  virtual void RemovePins(const std::string& key, RemovePinCallback callback);
  // Checks that all cids related to the key are pinned.
  virtual void ValidatePins(const std::string& key,
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
  raw_ptr<PrefService> prefs_service_;
  raw_ptr<IpfsService> ipfs_service_;

  base::WeakPtrFactory<IpfsLocalPinService> weak_ptr_factory_{this};
};

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_PIN_IPFS_LOCAL_PIN_SERVICE_H_
