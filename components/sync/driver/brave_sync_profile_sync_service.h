/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_SYNC_DRIVER_BRAVE_SYNC_PROFILE_SYNC_SERVICE_H_
#define BRAVE_COMPONENTS_SYNC_DRIVER_BRAVE_SYNC_PROFILE_SYNC_SERVICE_H_

#include <memory>
#include <string>

#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "brave/components/brave_sync/brave_sync_prefs.h"
#include "components/prefs/pref_change_registrar.h"
#include "components/sync/driver/profile_sync_service.h"

class Profile;

FORWARD_DECLARE_TEST(BraveProfileSyncServiceTest, ReenableTypes);
FORWARD_DECLARE_TEST(BraveProfileSyncServiceTest, ReenableTypesMaxPeriod);

namespace syncer {

class BraveSyncAuthManager;
class ProfileSyncServiceDelegate;

class BraveProfileSyncService : public ProfileSyncService {
 public:
  explicit BraveProfileSyncService(
      InitParams init_params,
      std::unique_ptr<ProfileSyncServiceDelegate> profile_service_delegate);
  ~BraveProfileSyncService() override;

  // SyncService implementation
  bool IsSetupInProgress() const override;

  std::string GetOrCreateSyncCode();
  bool SetSyncCode(const std::string& sync_code);

  // This should only be called by helper function, brave_sync::ResetSync, or by
  // OnDeviceInfoChange internally
  void OnSelfDeviceInfoDeleted(base::OnceClosure cb);

  // These functions are for disabling device_info_observer_ from firing
  // when the device is doing own reset sync operation, to prevent early call
  // of StopAndClear prior to device sends delete record
  void SuspendDeviceObserverForOwnReset();
  void ResumeDeviceObserver();

  void Initialize() override;

 private:
  FRIEND_TEST_ALL_PREFIXES(BraveProfileSyncServiceTest, ReenableTypes);
  FRIEND_TEST_ALL_PREFIXES(BraveProfileSyncServiceTest, ReenableTypesMaxPeriod);

  BraveSyncAuthManager* GetBraveSyncAuthManager();

  void OnBraveSyncPrefsChanged(const std::string& path);

  void OnSyncCycleCompleted(const SyncCycleSnapshot& snapshot) override;
  // Re-enables current sync types to to recover state from being
  // SERVER_RETURN_CONFLICT or SERVER_RETURN_TRANSIENT_ERROR
  void ReenableSyncTypes();
  bool IsReenableTypesRequired(const SyncCycleSnapshot& snapshot);
  size_t failed_commit_times_ = 0;
  base::Time last_reenable_types_time_;
  static size_t GetNumberOfFailedCommitsToReenableForTests();
  static base::TimeDelta MinimalTimeBetweenReenableForTests();

  brave_sync::Prefs brave_sync_prefs_;

  PrefChangeRegistrar brave_sync_prefs_change_registrar_;

  std::unique_ptr<ProfileSyncServiceDelegate> profile_service_delegate_;

  base::WeakPtrFactory<BraveProfileSyncService> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(BraveProfileSyncService);
};
}  // namespace syncer

#endif  // BRAVE_COMPONENTS_SYNC_DRIVER_BRAVE_SYNC_PROFILE_SYNC_SERVICE_H_
