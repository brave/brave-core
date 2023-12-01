// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_IPFS_PIN_IPFS_LOCAL_PIN_SERVICE_H_
#define BRAVE_COMPONENTS_IPFS_PIN_IPFS_LOCAL_PIN_SERVICE_H_

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/barrier_callback.h"
#include "base/functional/callback.h"
#include "base/gtest_prod_util.h"
#include "brave/components/ipfs/ipfs_service.h"
#include "brave/components/ipfs/pin/ipfs_base_pin_service.h"

using ipfs::IpfsService;

namespace ipfs {

enum PinningMode { DIRECT = 0, RECURSIVE = 1 };

struct PinData {
  std::string cid;
  PinningMode pinning_mode;

  bool operator==(const PinData&) const;
};

std::optional<std::vector<PinData>> ExtractPinData(const std::string& ipfs_url);

using AddPinCallback = base::OnceCallback<void(bool)>;
using RemovePinCallback = base::OnceCallback<void(bool)>;
using ValidatePinsCallback = base::OnceCallback<void(std::optional<bool>)>;
using GcCallback = base::OnceCallback<void(bool)>;

/**
 * Pins provided cids and writes record to kIPFSPinnedCids:
 * {
 *   // CIDs which were pinned recursively
 *   "recursive": {
 *     // List of all pinned CIDs
 *     "Qme1": [
 *       // List of tokens that contain this CID
 *       "nft.local.60.0x1.0xbc4ca0eda7647a8ab7c2061c2e118a18a936f13d.0x1"
 *       "nft.local.60.0x1.0xbc4ca0eda7647a8ab7c2061c2e118a18a936f13d.0x2"
 *     ],
 *     "Qme2": [
 *       "nft.local.60.0x1.0xbc4ca0eda7647a8ab7c2061c2e118a18a936f13d.0x1"
 *     ],
 *     "Qme3": [
 *       "nft.local.60.0x1.0xbc4ca0eda7647a8ab7c2061c2e118a18a936f13d.0x2"
 *     ]
 *   },
 *   // CIDs which were pinned using direct mode
 *   "direct": {
 *     ...
 *   }
 * }
 */
class AddLocalPinJob : public IpfsBaseJob {
 public:
  AddLocalPinJob(PrefService* prefs_service,
                 IpfsService* ipfs_service,
                 const std::string& key,
                 const std::vector<PinData>& pins_data,
                 AddPinCallback callback);
  ~AddLocalPinJob() override;

  void Start() override;

 private:
  void Accumulate(
      base::OnceCallback<void(std::optional<AddPinResult>)> callback,
      std::optional<AddPinResult> result);
  void OnAddPinResult(std::vector<std::optional<AddPinResult>> result);

  raw_ptr<PrefService> prefs_service_;
  raw_ptr<IpfsService> ipfs_service_;
  std::string key_;
  std::vector<PinData> pins_data_;
  AddPinCallback callback_;

  bool pinning_failed_ = false;

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
                    const std::vector<PinData>& PinData,
                    ValidatePinsCallback callback);
  ~VerifyLocalPinJob() override;

  void Start() override;

 private:
  void OnGetPinsResult(std::optional<GetPinsResult> result);

  raw_ptr<PrefService> prefs_service_;
  raw_ptr<IpfsService> ipfs_service_;
  std::string key_;
  std::vector<PinData> pins_data_;
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
  void Accumulate(
      base::OnceCallback<void(std::optional<GetPinsResult>)> callback,
      std::optional<GetPinsResult> result);
  void OnGetPinsResult(std::vector<std::optional<GetPinsResult>> result);
  void OnPinsRemovedResult(std::optional<RemovePinResult> result);

  raw_ptr<PrefService> prefs_service_;
  raw_ptr<IpfsService> ipfs_service_;
  GcCallback callback_;
  bool gc_job_failed_ = false;

  base::WeakPtrFactory<GcJob> weak_ptr_factory_{this};
};

class IpfsLocalPinService : public KeyedService {
 public:
  IpfsLocalPinService(PrefService* prefs_service, IpfsService* ipfs_service);
  // For testing
  IpfsLocalPinService();
  ~IpfsLocalPinService() override;

  virtual void Reset(base::OnceCallback<void(bool)> callback);

  // Pins provided cids and stores related record in the prefs.
  virtual void AddPins(const std::string& key,
                       const std::vector<std::string>& ipfs_urls,
                       AddPinCallback callback);
  // Unpins all cids related to the key.
  virtual void RemovePins(const std::string& key, RemovePinCallback callback);
  // Checks that all cids related to the key are pinned.
  virtual void ValidatePins(const std::string& key,
                            const std::vector<std::string>& ipfs_urls,
                            ValidatePinsCallback callback);
  void ScheduleGcTask();

  void SetIpfsBasePinServiceForTesting(std::unique_ptr<IpfsBasePinService>);

  static std::optional<std::vector<PinData>> ExtractPinData(
      const std::string& ipfs_url);
  static std::optional<std::vector<ipfs::PinData>> ExtractMergedPinData(
      const std::vector<std::string>& ipfs_urls);

 private:
  FRIEND_TEST_ALL_PREFIXES(IpfsLocalPinServiceTest, ResetTest);

  void OnLsPinCliResult(base::OnceCallback<void(bool)> callback,
                        std::optional<std::string> result);
  void OnRemovePinCliResult(base::OnceCallback<void(bool)> callback,
                            bool result);
  void AddGcTask();
  void OnRemovePinsFinished(RemovePinCallback callback, bool status);
  void OnAddJobFinished(AddPinCallback callback, bool status);
  void OnValidateJobFinished(ValidatePinsCallback callback,
                             std::optional<bool> status);
  void OnGcFinishedCallback(bool status);
  bool HasJobs();

  bool gc_task_posted_ = false;
  std::unique_ptr<IpfsBasePinService> ipfs_base_pin_service_;
  raw_ptr<PrefService> prefs_service_;
  raw_ptr<IpfsService> ipfs_service_;

  base::WeakPtrFactory<IpfsLocalPinService> weak_ptr_factory_{this};
};

}  // namespace ipfs

#endif  // BRAVE_COMPONENTS_IPFS_PIN_IPFS_LOCAL_PIN_SERVICE_H_
