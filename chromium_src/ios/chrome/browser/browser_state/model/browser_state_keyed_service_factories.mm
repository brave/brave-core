/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "ios/chrome/browser/browser_state/model/browser_state_keyed_service_factories.h"

#include "ios/chrome/browser/autofill/personal_data_manager_factory.h"
#include "ios/chrome/browser/bookmarks/model/bookmark_undo_service_factory.h"
#include "ios/chrome/browser/bookmarks/model/local_or_syncable_bookmark_model_factory.h"
#include "ios/chrome/browser/consent_auditor/consent_auditor_factory.h"
#include "ios/chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "ios/chrome/browser/favicon/favicon_service_factory.h"
#include "ios/chrome/browser/favicon/ios_chrome_favicon_loader_factory.h"
#include "ios/chrome/browser/favicon/ios_chrome_large_icon_cache_factory.h"
#include "ios/chrome/browser/favicon/ios_chrome_large_icon_service_factory.h"
#include "ios/chrome/browser/history/history_service_factory.h"
#include "ios/chrome/browser/history/web_history_service_factory.h"
#include "ios/chrome/browser/invalidation/ios_chrome_profile_invalidation_provider_factory.h"
#include "ios/chrome/browser/passwords/ios_chrome_password_store_factory.h"
#include "ios/chrome/browser/reading_list/reading_list_model_factory.h"
#include "ios/chrome/browser/search_engines/template_url_service_factory.h"
#include "ios/chrome/browser/signin/account_consistency_service_factory.h"
#include "ios/chrome/browser/signin/identity_manager_factory.h"
#include "ios/chrome/browser/sync/ios_user_event_service_factory.h"
#include "ios/chrome/browser/sync/model_type_store_service_factory.h"
#include "ios/chrome/browser/sync/session_sync_service_factory.h"
#include "ios/chrome/browser/sync/sync_service_factory.h"
#include "ios/chrome/browser/sync/sync_setup_service_factory.h"
#include "ios/chrome/browser/webdata_services/web_data_service_factory.h"

void EnsureBrowserStateKeyedServiceFactoriesBuilt() {
  autofill::PersonalDataManagerFactory::GetInstance();
  ConsentAuditorFactory::GetInstance();
  ios::AccountConsistencyServiceFactory::GetInstance();
  ios::LocalOrSyncableBookmarkModelFactory::GetInstance();
  ios::BookmarkUndoServiceFactory::GetInstance();
  ios::FaviconServiceFactory::GetInstance();
  ios::HistoryServiceFactory::GetInstance();
  ios::HostContentSettingsMapFactory::GetInstance();
  ios::TemplateURLServiceFactory::GetInstance();
  ios::WebDataServiceFactory::GetInstance();
  ios::WebHistoryServiceFactory::GetInstance();
  IdentityManagerFactory::GetInstance();
  IOSChromeFaviconLoaderFactory::GetInstance();
  IOSChromeLargeIconCacheFactory::GetInstance();
  IOSChromeLargeIconServiceFactory::GetInstance();
  IOSChromePasswordStoreFactory::GetInstance();
  IOSChromeProfileInvalidationProviderFactory::GetInstance();
  IOSUserEventServiceFactory::GetInstance();
  ModelTypeStoreServiceFactory::GetInstance();
  SyncServiceFactory::GetInstance();
  ReadingListModelFactory::GetInstance();
  SessionSyncServiceFactory::GetInstance();
  SyncSetupServiceFactory::GetInstance();
}
