/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_IOS_BROWSER_API_SYNC_BRAVE_SYNC_WORKER_H_
#define BRAVE_IOS_BROWSER_API_SYNC_BRAVE_SYNC_WORKER_H_

#include <functional>
#include <memory>
#include <string>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "base/scoped_multi_source_observation.h"
#include "base/scoped_observation.h"
#include "brave/components/brave_sync/time_limited_words.h"
#include "components/sync/engine/sync_protocol_error.h"
#include "components/sync/service/sync_service.h"
#include "components/sync/service/sync_service_observer.h"
#include "components/sync_device_info/device_info_sync_service.h"
#include "components/sync_device_info/device_info_tracker.h"

class ChromeBrowserState;
struct SyncProtocolError;

namespace syncer {
class BraveSyncServiceImpl;
class DeviceInfo;
class BraveDeviceInfo;
class SyncServiceImpl;
}  // namespace syncer

namespace brave_sync {
enum class QrCodeDataValidationResult;
}  // namespace brave_sync

class BraveSyncDeviceTracker : public syncer::DeviceInfoTracker::Observer {
 public:
  BraveSyncDeviceTracker(
      syncer::DeviceInfoTracker* device_info_tracker,
      const base::RepeatingCallback<void()>& on_device_info_changed_callback);
  ~BraveSyncDeviceTracker() override;

 private:
  void OnDeviceInfoChange() override;

  base::RepeatingCallback<void()> on_device_info_changed_callback_;

  base::ScopedObservation<syncer::DeviceInfoTracker,
                          syncer::DeviceInfoTracker::Observer>
      device_info_tracker_observer_{this};
};

class BraveSyncServiceTracker : public syncer::SyncServiceObserver {
 public:
  BraveSyncServiceTracker(
      syncer::SyncServiceImpl* sync_service_impl,
      const base::RepeatingCallback<void()>& on_state_changed_callback,
      const base::RepeatingCallback<void()>& on_sync_shutdown_callback);
  ~BraveSyncServiceTracker() override;

 private:
  void OnStateChanged(syncer::SyncService* sync) override;
  void OnSyncShutdown(syncer::SyncService* sync) override;

  base::RepeatingCallback<void()> on_state_changed_callback_;
  base::RepeatingCallback<void()> on_sync_shutdown_callback_;

  base::ScopedObservation<syncer::SyncService, syncer::SyncServiceObserver>
      sync_service_observer_{this};
};

class BraveSyncWorker : public syncer::SyncServiceObserver {
 public:
  explicit BraveSyncWorker(ChromeBrowserState* browser_state_);
  BraveSyncWorker(const BraveSyncWorker&) = delete;
  BraveSyncWorker& operator=(const BraveSyncWorker&) = delete;
  ~BraveSyncWorker() override;

  bool RequestSync();
  std::string GetOrCreateSyncCode();
  bool IsValidSyncCode(const std::string& sync_code);
  bool SetSyncCode(const std::string& sync_code);
  std::string GetSyncCodeFromHexSeed(const std::string& hex_seed);
  std::string GetHexSeedFromSyncCode(const std::string& code_words);
  std::string GetQrCodeJsonFromHexSeed(const std::string& hex_seed);
  brave_sync::QrCodeDataValidationResult GetQrCodeValidationResult(
      const std::string json);
  brave_sync::TimeLimitedWords::ValidationStatus GetWordsValidationResult(
      const std::string time_limited_words);
  std::string GetWordsFromTimeLimitedWords(
      const std::string& time_limited_words);
  std::string GetTimeLimitedWordsFromWords(const std::string& words);
  std::string GetHexSeedFromQrCodeJson(const std::string& json);
  const syncer::DeviceInfo* GetLocalDeviceInfo();
  std::vector<std::unique_ptr<syncer::BraveDeviceInfo>> GetDeviceList();
  bool CanSyncFeatureStart();
  bool IsSyncFeatureActive();
  bool IsInitialSyncFeatureSetupComplete();
  bool SetSetupComplete();
  void ResetSync();
  void DeleteDevice(const std::string& device_guid);
  void SetJoinSyncChainCallback(base::OnceCallback<void(bool)> callback);
  void PermanentlyDeleteAccount(
      base::OnceCallback<void(const syncer::SyncProtocolError&)> callback);

 private:
  // syncer::SyncServiceObserver implementation.

  syncer::BraveSyncServiceImpl* GetSyncService() const;
  void OnStateChanged(syncer::SyncService* service) override;
  void OnSyncShutdown(syncer::SyncService* service) override;

  void OnResetDone();

  void SetEncryptionPassphrase(syncer::SyncService* service);
  void SetDecryptionPassphrase(syncer::SyncService* service);

  std::string passphrase_;

  raw_ptr<ChromeBrowserState> browser_state_;  // NOT OWNED
  base::ScopedMultiSourceObservation<syncer::SyncService,
                                     syncer::SyncServiceObserver>
      sync_service_observer_{this};
  base::WeakPtrFactory<BraveSyncWorker> weak_ptr_factory_{this};
};

#endif  // BRAVE_IOS_BROWSER_API_SYNC_BRAVE_SYNC_WORKER_H_
