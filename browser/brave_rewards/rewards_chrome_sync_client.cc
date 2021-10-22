/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_rewards/rewards_chrome_sync_client.h"

#include <memory>
#include <utility>

#include "base/files/file_path.h"
#include "base/time/default_clock.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sync/device_info_sync_client_impl.h"
#include "chrome/common/channel_info.h"
#include "components/sync/base/model_type.h"
#include "components/sync/base/report_unrecoverable_error.h"
#include "components/sync/driver/model_type_controller.h"
#include "components/sync/driver/syncable_service_based_model_type_controller.h"
#include "components/sync/model/forwarding_model_type_controller_delegate.h"
#include "components/sync/model/model_type_store_service_impl.h"
#include "components/sync_device_info/device_info_prefs.h"
#include "components/sync_device_info/device_info_sync_service_impl.h"
#include "components/sync_device_info/local_device_info_provider_impl.h"
#include "components/sync_preferences/pref_service_syncable.h"

using browser_sync::DeviceInfoSyncClientImpl;
using sync_preferences::PrefServiceSyncable;
using syncer::DeviceInfoPrefs;
using syncer::DeviceInfoSyncServiceImpl;
using syncer::ForwardingModelTypeControllerDelegate;
using syncer::LocalDeviceInfoProviderImpl;
using syncer::ModelTypeController;
using syncer::ModelTypeStoreServiceImpl;
using syncer::SyncableServiceBasedModelTypeController;

namespace {
auto CreateScopedPrefServiceSyncable(
    PrefServiceSyncable* pref_service_syncable) {
  DCHECK(pref_service_syncable);
  return pref_service_syncable->CreateScopedPrefService(nullptr, {});
}

auto CreateModelTypeStoreService(Profile* profile) {
  DCHECK(profile);
  return std::make_unique<ModelTypeStoreServiceImpl>(
      profile->GetPath().AppendASCII("rewards_sync"));
}

auto CreateDeviceInfoSyncService(
    Profile* profile,
    PrefServiceSyncable* pref_service_syncable,
    const std::unique_ptr<ModelTypeStoreServiceImpl>&
        model_type_store_service) {
  DCHECK(profile);
  DCHECK(pref_service_syncable);
  DCHECK(model_type_store_service);

  auto device_info_sync_client =
      std::make_unique<DeviceInfoSyncClientImpl>(profile);

  auto local_device_info_provider =
      std::make_unique<LocalDeviceInfoProviderImpl>(
          chrome::GetChannel(),
          chrome::GetVersionString(chrome::WithExtendedStable(false)),
          device_info_sync_client.get());

  auto device_prefs = std::make_unique<DeviceInfoPrefs>(
      pref_service_syncable, base::DefaultClock::GetInstance());

  return std::make_unique<DeviceInfoSyncServiceImpl>(
      model_type_store_service->GetStoreFactory(),
      std::move(local_device_info_provider), std::move(device_prefs),
      std::move(device_info_sync_client), nullptr);
}
}  // namespace

namespace browser_sync {
RewardsChromeSyncClient::RewardsChromeSyncClient(Profile* profile)
    : ChromeSyncClient(profile),
      scoped_pref_service_syncable_(CreateScopedPrefServiceSyncable(
          ChromeSyncClient::GetPrefServiceSyncable())),
      model_type_store_service_(CreateModelTypeStoreService(profile)),
      device_info_sync_service_(CreateDeviceInfoSyncService(
          profile,
          ChromeSyncClient::GetPrefServiceSyncable(),
          model_type_store_service_)) {}

RewardsChromeSyncClient::~RewardsChromeSyncClient() = default;

syncer::DataTypeController::TypeVector
RewardsChromeSyncClient::CreateDataTypeControllers(syncer::SyncService*) {
  syncer::DataTypeController::TypeVector controllers;

  controllers.push_back(std::make_unique<ModelTypeController>(
      syncer::DEVICE_INFO,
      std::make_unique<ForwardingModelTypeControllerDelegate>(
          device_info_sync_service_->GetControllerDelegate().get()),
      std::make_unique<ForwardingModelTypeControllerDelegate>(
          device_info_sync_service_->GetControllerDelegate().get())));

  return controllers;
}

PrefService* RewardsChromeSyncClient::GetPrefService() {
  DCHECK(scoped_pref_service_syncable_);
  return scoped_pref_service_syncable_.get();
}
}  // namespace browser_sync
