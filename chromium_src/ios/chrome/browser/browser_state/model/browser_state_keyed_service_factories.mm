/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ios/chrome/browser/browser_state/model/browser_state_keyed_service_factories.h"

#include "brave/ios/browser/browser_state/brave_browser_state_keyed_service_factories.h"
#include "ios/chrome/browser/autocomplete/model/autocomplete_classifier_factory.h"
#include "ios/chrome/browser/autofill/model/personal_data_manager_factory.h"
#include "ios/chrome/browser/bookmarks/model/bookmark_model_factory.h"
#include "ios/chrome/browser/bookmarks/model/bookmark_undo_service_factory.h"
#include "ios/chrome/browser/consent_auditor/model/consent_auditor_factory.h"
#include "ios/chrome/browser/content_settings/model/host_content_settings_map_factory.h"
#include "ios/chrome/browser/credential_provider/model/credential_provider_buildflags.h"
#include "ios/chrome/browser/favicon/model/favicon_service_factory.h"
#include "ios/chrome/browser/favicon/model/ios_chrome_favicon_loader_factory.h"
#include "ios/chrome/browser/favicon/model/ios_chrome_large_icon_cache_factory.h"
#include "ios/chrome/browser/favicon/model/ios_chrome_large_icon_service_factory.h"
#include "ios/chrome/browser/history/model/history_service_factory.h"
#include "ios/chrome/browser/history/model/top_sites_factory.h"
#include "ios/chrome/browser/history/model/web_history_service_factory.h"
#include "ios/chrome/browser/invalidation/model/ios_chrome_profile_invalidation_provider_factory.h"
#include "ios/chrome/browser/passwords/model/ios_chrome_profile_password_store_factory.h"
#include "ios/chrome/browser/reading_list/model/reading_list_model_factory.h"
#include "ios/chrome/browser/search_engines/model/template_url_service_factory.h"
#include "ios/chrome/browser/sessions/session_restoration_service_factory.h"
#include "ios/chrome/browser/signin/model/account_consistency_service_factory.h"
#include "ios/chrome/browser/signin/model/identity_manager_factory.h"
#include "ios/chrome/browser/sync/model/ios_user_event_service_factory.h"
#include "ios/chrome/browser/sync/model/model_type_store_service_factory.h"
#include "ios/chrome/browser/sync/model/session_sync_service_factory.h"
#include "ios/chrome/browser/sync/model/sync_service_factory.h"
#include "ios/chrome/browser/webdata_services/model/web_data_service_factory.h"

#if BUILDFLAG(IOS_CREDENTIAL_PROVIDER_ENABLED)
#import "ios/chrome/browser/credential_provider/model/credential_provider_service_factory.h"
#endif

void EnsureBrowserStateKeyedServiceFactoriesBuilt() {
  autofill::PersonalDataManagerFactory::GetInstance();
  ConsentAuditorFactory::GetInstance();
  ios::AccountConsistencyServiceFactory::GetInstance();
  ios::BookmarkModelFactory::GetInstance();
  ios::BookmarkUndoServiceFactory::GetInstance();
  ios::FaviconServiceFactory::GetInstance();
  ios::HistoryServiceFactory::GetInstance();
  ios::TopSitesFactory::GetInstance();
  ios::AutocompleteClassifierFactory::GetInstance();
  ios::HostContentSettingsMapFactory::GetInstance();
  ios::TemplateURLServiceFactory::GetInstance();
  ios::WebDataServiceFactory::GetInstance();
  ios::WebHistoryServiceFactory::GetInstance();
  IdentityManagerFactory::GetInstance();
  IOSChromeFaviconLoaderFactory::GetInstance();
  IOSChromeLargeIconCacheFactory::GetInstance();
  IOSChromeLargeIconServiceFactory::GetInstance();
  IOSChromeProfilePasswordStoreFactory::GetInstance();
  IOSChromeProfileInvalidationProviderFactory::GetInstance();
  IOSUserEventServiceFactory::GetInstance();
  ModelTypeStoreServiceFactory::GetInstance();
  SyncServiceFactory::GetInstance();
  ReadingListModelFactory::GetInstance();
  SessionRestorationServiceFactory::GetInstance();
  SessionSyncServiceFactory::GetInstance();

#if BUILDFLAG(IOS_CREDENTIAL_PROVIDER_ENABLED)
  CredentialProviderServiceFactory::GetInstance();
#endif

  brave::EnsureBrowserStateKeyedServiceFactoriesBuilt();
}
