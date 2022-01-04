/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_rewards/vg_sync_service_factory.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/memory/singleton.h"
#include "brave/browser/brave_rewards/vg_body_sync_bridge.h"
#include "brave/browser/brave_rewards/vg_spend_status_sync_bridge.h"
#include "brave/browser/brave_rewards/vg_sync_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sync/model_type_store_service_factory.h"
#include "chrome/common/channel_info.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/sync/base/report_unrecoverable_error.h"
#include "components/sync/model/client_tag_based_model_type_processor.h"
#include "components/sync/model/model_type_store_service.h"

// static
VgSyncServiceFactory* VgSyncServiceFactory::GetInstance() {
  return base::Singleton<VgSyncServiceFactory>::get();
}

// static
VgSyncService* VgSyncServiceFactory::GetForProfile(Profile* profile) {
  return static_cast<VgSyncService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}

VgSyncServiceFactory::VgSyncServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "VgSyncService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(ModelTypeStoreServiceFactory::GetInstance());
}

VgSyncServiceFactory::~VgSyncServiceFactory() = default;

KeyedService* VgSyncServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  auto vg_body_change_processor =
      std::make_unique<syncer::ClientTagBasedModelTypeProcessor>(
          syncer::VG_BODIES,
          base::BindRepeating(&syncer::ReportUnrecoverableError,
                              chrome::GetChannel()));
  auto vg_body_store_factory = ModelTypeStoreServiceFactory::GetForProfile(
                                   static_cast<Profile*>(context))
                                   ->GetStoreFactory();
  auto vg_body_sync_bridge = std::make_unique<VgBodySyncBridge>(
      std::move(vg_body_change_processor), std::move(vg_body_store_factory));

  auto vg_spend_status_change_processor =
      std::make_unique<syncer::ClientTagBasedModelTypeProcessor>(
          syncer::VG_SPEND_STATUSES,
          base::BindRepeating(&syncer::ReportUnrecoverableError,
                              chrome::GetChannel()));
  auto vg_spend_status_store_factory =
      ModelTypeStoreServiceFactory::GetForProfile(
          static_cast<Profile*>(context))
          ->GetStoreFactory();
  auto vg_spend_status_sync_bridge = std::make_unique<VgSpendStatusSyncBridge>(
      std::move(vg_spend_status_change_processor),
      std::move(vg_spend_status_store_factory));

  return new VgSyncService(std::move(vg_body_sync_bridge),
                           std::move(vg_spend_status_sync_bridge));
}
