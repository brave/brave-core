/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_REWARDS_REWARDS_CHROME_SYNC_CLIENT_H_
#define BRAVE_BROWSER_BRAVE_REWARDS_REWARDS_CHROME_SYNC_CLIENT_H_

#include <memory>

#include "chrome/browser/sync/chrome_sync_client.h"
#include "components/sync/driver/data_type_controller.h"

class PrefService;
class Profile;

namespace invalidation {
class ProfileInvalidationProvider;
}

namespace sync_preferences {
class PrefServiceSyncable;
}  // namespace sync_preferences

namespace syncer {
class DeviceInfoSyncServiceImpl;
class ModelTypeStoreServiceImpl;
class SyncService;
}  // namespace syncer

namespace browser_sync {
class RewardsChromeSyncClient : public ChromeSyncClient {
 public:
  explicit RewardsChromeSyncClient(Profile* profile);

  ~RewardsChromeSyncClient() override;

  PrefService* GetPrefService() override;

  syncer::DataTypeController::TypeVector CreateDataTypeControllers(
      syncer::SyncService* sync_service) override;

  invalidation::InvalidationService* GetInvalidationService() override;

  void SetDefaultEnabledTypes(syncer::SyncService* sync_service) override;

 private:
  std::unique_ptr<sync_preferences::PrefServiceSyncable>
      scoped_pref_service_syncable_;
  std::unique_ptr<syncer::ModelTypeStoreServiceImpl> model_type_store_service_;
  std::unique_ptr<syncer::DeviceInfoSyncServiceImpl> device_info_sync_service_;
  std::unique_ptr<invalidation::ProfileInvalidationProvider>
      profile_invalidation_provider_;
};
}  // namespace browser_sync

#endif  // BRAVE_BROWSER_BRAVE_REWARDS_REWARDS_CHROME_SYNC_CLIENT_H_
