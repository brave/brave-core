/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/sync_pairs/pair_sync_service_factory.h"

#include <memory>
#include <utility>

#include "base/bind.h"
#include "base/memory/singleton.h"
#include "brave/browser/sync_pairs/pair_sync_bridge.h"
#include "brave/browser/sync_pairs/pair_sync_service.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/sync/model_type_store_service_factory.h"
#include "chrome/common/channel_info.h"
#include "components/keyed_service/content/browser_context_dependency_manager.h"
#include "components/sync/base/report_unrecoverable_error.h"
#include "components/sync/model/client_tag_based_model_type_processor.h"
#include "components/sync/model/model_type_store_service.h"

// static
PairSyncServiceFactory* PairSyncServiceFactory::GetInstance() {
  return base::Singleton<PairSyncServiceFactory>::get();
}

// static
PairSyncService* PairSyncServiceFactory::GetForProfile(Profile* profile) {
  return static_cast<PairSyncService*>(
      GetInstance()->GetServiceForBrowserContext(profile, true));
}
PairSyncServiceFactory::PairSyncServiceFactory()
    : BrowserContextKeyedServiceFactory(
          "PairSyncService",
          BrowserContextDependencyManager::GetInstance()) {
  DependsOn(ModelTypeStoreServiceFactory::GetInstance());
}

PairSyncServiceFactory::~PairSyncServiceFactory() = default;

KeyedService* PairSyncServiceFactory::BuildServiceInstanceFor(
    content::BrowserContext* context) const {
  auto change_processor =
      std::make_unique<syncer::ClientTagBasedModelTypeProcessor>(
          syncer::PAIRS, base::BindRepeating(&syncer::ReportUnrecoverableError,
                                             chrome::GetChannel()));
  auto store_factory = ModelTypeStoreServiceFactory::GetForProfile(
                           static_cast<Profile*>(context))
                           ->GetStoreFactory();
  auto pair_sync_bridge = std::make_unique<PairSyncBridge>(
      std::move(change_processor), std::move(store_factory));

  return new PairSyncService(std::move(pair_sync_bridge));
}
