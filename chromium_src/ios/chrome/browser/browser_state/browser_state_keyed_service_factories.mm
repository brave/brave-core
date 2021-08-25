/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ios/chrome/browser/browser_state/browser_state_keyed_service_factories.h"

#include "brave/ios/browser/brave_wallet/asset_ratio_controller_factory.h"
#include "brave/ios/browser/brave_wallet/eth_json_rpc_controller_factory.h"
#include "brave/ios/browser/brave_wallet/keyring_controller_factory.h"
#include "ios/chrome/browser/autofill/personal_data_manager_factory.h"
#include "ios/chrome/browser/bookmarks/bookmark_model_factory.h"
#include "ios/chrome/browser/favicon/favicon_service_factory.h"
#include "ios/chrome/browser/history/history_service_factory.h"
#include "ios/chrome/browser/history/web_history_service_factory.h"
#include "ios/chrome/browser/invalidation/ios_chrome_profile_invalidation_provider_factory.h"
#include "ios/chrome/browser/passwords/ios_chrome_password_store_factory.h"
#include "ios/chrome/browser/reading_list/reading_list_model_factory.h"
#include "ios/chrome/browser/search_engines/template_url_service_factory.h"
#include "ios/chrome/browser/signin/account_consistency_service_factory.h"
#include "ios/chrome/browser/signin/identity_manager_factory.h"
#include "ios/chrome/browser/sync/consent_auditor_factory.h"
#include "ios/chrome/browser/sync/ios_user_event_service_factory.h"
#include "ios/chrome/browser/sync/model_type_store_service_factory.h"
#include "ios/chrome/browser/sync/session_sync_service_factory.h"
#include "ios/chrome/browser/sync/sync_service_factory.h"
#include "ios/chrome/browser/sync/sync_setup_service_factory.h"
#include "ios/chrome/browser/undo/bookmark_undo_service_factory.h"
#include "ios/chrome/browser/webdata_services/web_data_service_factory.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

void EnsureBrowserStateKeyedServiceFactoriesBuilt() {
  autofill::PersonalDataManagerFactory::GetInstance();
  brave_wallet::KeyringControllerFactory::GetInstance();
  brave_wallet::AssetRatioControllerFactory::GetInstance();
  brave_wallet::EthJsonRpcControllerFactory::GetInstance();
  ConsentAuditorFactory::GetInstance();
  ios::AccountConsistencyServiceFactory::GetInstance();
  ios::BookmarkModelFactory::GetInstance();
  ios::BookmarkUndoServiceFactory::GetInstance();
  ios::FaviconServiceFactory::GetInstance();
  ios::HistoryServiceFactory::GetInstance();
  ios::TemplateURLServiceFactory::GetInstance();
  ios::WebDataServiceFactory::GetInstance();
  ios::WebHistoryServiceFactory::GetInstance();
  IdentityManagerFactory::GetInstance();
  IOSChromePasswordStoreFactory::GetInstance();
  IOSChromeProfileInvalidationProviderFactory::GetInstance();
  IOSUserEventServiceFactory::GetInstance();
  ModelTypeStoreServiceFactory::GetInstance();
  SyncServiceFactory::GetInstance();
  ReadingListModelFactory::GetInstance();
  SessionSyncServiceFactory::GetInstance();
  SyncSetupServiceFactory::GetInstance();
}
