/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_rewards/rewards_chrome_sync_client.h"

#include <memory>
#include <utility>

#include "base/files/file_path.h"
#include "base/time/default_clock.h"
#include "chrome/browser/gcm/gcm_profile_service_factory.h"
#include "chrome/browser/gcm/instance_id/instance_id_profile_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/signin/identity_manager_factory.h"
#include "chrome/browser/sync/device_info_sync_client_impl.h"
#include "chrome/common/channel_info.h"
#include "components/gcm_driver/gcm_profile_service.h"
#include "components/gcm_driver/instance_id/instance_id_profile_service.h"
#include "components/invalidation/impl/fcm_invalidation_service.h"
#include "components/invalidation/impl/fcm_network_handler.h"
#include "components/invalidation/impl/per_user_topic_subscription_manager.h"
#include "components/invalidation/impl/profile_identity_provider.h"
#include "components/invalidation/impl/profile_invalidation_provider.h"
#include "components/invalidation/public/identity_provider.h"
#include "components/invalidation/public/invalidation_service.h"
#include "components/sync/base/model_type.h"
#include "components/sync/base/report_unrecoverable_error.h"
#include "components/sync/driver/model_type_controller.h"
#include "components/sync/driver/sync_service.h"
#include "components/sync/driver/sync_user_settings.h"
#include "components/sync/driver/syncable_service_based_model_type_controller.h"
#include "components/sync/model/forwarding_model_type_controller_delegate.h"
#include "components/sync/model/model_type_store_service_impl.h"
#include "components/sync_device_info/device_info_prefs.h"
#include "components/sync_device_info/device_info_sync_service_impl.h"
#include "components/sync_device_info/local_device_info_provider_impl.h"
#include "components/sync_preferences/pref_service_syncable.h"
#include "content/public/browser/storage_partition.h"

using browser_sync::DeviceInfoSyncClientImpl;
using invalidation::FCMInvalidationService;
using invalidation::FCMNetworkHandler;
using invalidation::IdentityProvider;
using invalidation::InvalidationService;
using invalidation::PerUserTopicSubscriptionManager;
using invalidation::ProfileIdentityProvider;
using invalidation::ProfileInvalidationProvider;
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

std::unique_ptr<InvalidationService> CreateInvalidationServiceForSenderId(
    Profile* profile,
    IdentityProvider* identity_provider,
    const std::string& sender_id) {
  auto service = std::make_unique<FCMInvalidationService>(
      identity_provider,
      base::BindRepeating(
          &FCMNetworkHandler::Create,
          gcm::GCMProfileServiceFactory::GetForProfile(profile)->driver(),
          instance_id::InstanceIDProfileServiceFactory::GetForProfile(profile)
              ->driver()),
      base::BindRepeating(
          &PerUserTopicSubscriptionManager::Create, identity_provider,
          profile->GetPrefs(),
          base::RetainedRef(profile->GetDefaultStoragePartition()
                                ->GetURLLoaderFactoryForBrowserProcess())),
      instance_id::InstanceIDProfileServiceFactory::GetForProfile(profile)
          ->driver(),
      profile->GetPrefs(), sender_id);
  service->Init();
  return service;
}

auto CreateProfileInvalidationProvider(Profile* profile) {
  auto identity_provider = std::make_unique<ProfileIdentityProvider>(
      IdentityManagerFactory::GetForProfile(profile));

  auto service =
      CreateInvalidationServiceForSenderId(profile, identity_provider.get(),
                                           /* sender_id = */ "");
  auto custom_sender_id_factory = base::BindRepeating(
      &CreateInvalidationServiceForSenderId, profile, identity_provider.get());
  return std::make_unique<ProfileInvalidationProvider>(
      std::move(service), std::move(identity_provider),
      std::move(custom_sender_id_factory));
}
}  // namespace

namespace browser_sync {
RewardsChromeSyncClient::RewardsChromeSyncClient(Profile* profile)
    : ChromeSyncClient(profile),
      scoped_pref_service_syncable_(CreateScopedPrefServiceSyncable(
          ChromeSyncClient::GetPrefServiceSyncable())),
      model_type_store_service_(CreateModelTypeStoreService(profile)),
      device_info_sync_service_(
          CreateDeviceInfoSyncService(profile,
                                      scoped_pref_service_syncable_.get(),
                                      model_type_store_service_)),
      profile_invalidation_provider_(
          CreateProfileInvalidationProvider(profile)) {}

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

  controllers.push_back(std::make_unique<ModelTypeController>(
      syncer::VG_BODIES,
      std::make_unique<syncer::ForwardingModelTypeControllerDelegate>(
          GetControllerDelegateForModelType(syncer::VG_BODIES).get()),
      std::make_unique<syncer::ForwardingModelTypeControllerDelegate>(
          GetControllerDelegateForModelType(syncer::VG_BODIES).get())));

  controllers.push_back(std::make_unique<ModelTypeController>(
      syncer::VG_SPEND_STATUSES,
      std::make_unique<syncer::ForwardingModelTypeControllerDelegate>(
          GetControllerDelegateForModelType(syncer::VG_SPEND_STATUSES).get()),
      std::make_unique<syncer::ForwardingModelTypeControllerDelegate>(
          GetControllerDelegateForModelType(syncer::VG_SPEND_STATUSES).get())));

  return controllers;
}

InvalidationService* RewardsChromeSyncClient::GetInvalidationService() {
  DCHECK(profile_invalidation_provider_);
  return profile_invalidation_provider_
             ? profile_invalidation_provider_->GetInvalidationService()
             : nullptr;
}

void RewardsChromeSyncClient::SetDefaultEnabledTypes(
    syncer::SyncService* sync_service) {
  DCHECK(sync_service);

  syncer::UserSelectableTypeSet selected_types;
  selected_types.Put(syncer::UserSelectableType::kVgBodies);
  selected_types.Put(syncer::UserSelectableType::kVgSpendStatuses);
  sync_service->GetUserSettings()->SetSelectedTypes(false, selected_types);
}

PrefService* RewardsChromeSyncClient::GetPrefService() {
  DCHECK(scoped_pref_service_syncable_);
  return scoped_pref_service_syncable_.get();
}
}  // namespace browser_sync
