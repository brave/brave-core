/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "ios/chrome/browser/browser_state/model/browser_state_keyed_service_factories.h"

#include "brave/ios/browser/browser_state/brave_browser_state_keyed_service_factories.h"
#include "ios/chrome/browser/autocomplete/model/autocomplete_classifier_factory.h"
#include "ios/chrome/browser/autocomplete/model/zero_suggest_cache_service_factory.h"
#include "ios/chrome/browser/autofill/model/personal_data_manager_factory.h"
#include "ios/chrome/browser/bookmarks/model/account_bookmark_sync_service_factory.h"
#include "ios/chrome/browser/bookmarks/model/bookmark_model_factory.h"
#include "ios/chrome/browser/bookmarks/model/bookmark_undo_service_factory.h"
#include "ios/chrome/browser/bookmarks/model/local_or_syncable_bookmark_sync_service_factory.h"
#include "ios/chrome/browser/consent_auditor/model/consent_auditor_factory.h"
#include "ios/chrome/browser/content_settings/model/host_content_settings_map_factory.h"
#include "ios/chrome/browser/credential_provider/model/credential_provider_buildflags.h"
#include "ios/chrome/browser/data_sharing/model/data_sharing_service_factory.h"
#include "ios/chrome/browser/download/model/background_service/background_download_service_factory.h"
#include "ios/chrome/browser/favicon/model/favicon_service_factory.h"
#include "ios/chrome/browser/favicon/model/ios_chrome_favicon_loader_factory.h"
#include "ios/chrome/browser/favicon/model/ios_chrome_large_icon_cache_factory.h"
#include "ios/chrome/browser/favicon/model/ios_chrome_large_icon_service_factory.h"
#include "ios/chrome/browser/history/model/history_service_factory.h"
#include "ios/chrome/browser/history/model/top_sites_factory.h"
#include "ios/chrome/browser/history/model/web_history_service_factory.h"
#include "ios/chrome/browser/invalidation/model/ios_chrome_profile_invalidation_provider_factory.h"
#include "ios/chrome/browser/metrics/model/google_groups_manager_factory.h"
#include "ios/chrome/browser/optimization_guide/model/optimization_guide_service_factory.h"
#include "ios/chrome/browser/page_info/about_this_site_service_factory.h"
#include "ios/chrome/browser/passwords/model/ios_chrome_account_password_store_factory.h"
#include "ios/chrome/browser/passwords/model/ios_chrome_password_receiver_service_factory.h"
#include "ios/chrome/browser/passwords/model/ios_chrome_password_reuse_manager_factory.h"
#include "ios/chrome/browser/passwords/model/ios_chrome_password_sender_service_factory.h"
#include "ios/chrome/browser/passwords/model/ios_chrome_profile_password_store_factory.h"
#include "ios/chrome/browser/passwords/model/ios_password_manager_settings_service_factory.h"
#include "ios/chrome/browser/plus_addresses/model/plus_address_service_factory.h"
#include "ios/chrome/browser/plus_addresses/model/plus_address_setting_service_factory.h"
#include "ios/chrome/browser/power_bookmarks/model/power_bookmark_service_factory.h"
#include "ios/chrome/browser/push_notification/model/push_notification_profile_service_factory.h"
#include "ios/chrome/browser/reading_list/model/reading_list_model_factory.h"
#include "ios/chrome/browser/safe_browsing/model/safe_browsing_metrics_collector_factory.h"
#include "ios/chrome/browser/saved_tab_groups/model/tab_group_sync_service_factory.h"
#include "ios/chrome/browser/search_engines/model/template_url_service_factory.h"
#include "ios/chrome/browser/segmentation_platform/model/segmentation_platform_service_factory.h"
#include "ios/chrome/browser/sessions/model/session_restoration_service_factory.h"
#include "ios/chrome/browser/shared/model/browser/browser_list_factory.h"
#include "ios/chrome/browser/signin/model/account_consistency_service_factory.h"
#include "ios/chrome/browser/signin/model/account_reconcilor_factory.h"
#include "ios/chrome/browser/signin/model/identity_manager_factory.h"
#include "ios/chrome/browser/supervised_user/model/child_account_service_factory.h"
#include "ios/chrome/browser/supervised_user/model/list_family_members_service_factory.h"
#include "ios/chrome/browser/supervised_user/model/supervised_user_service_factory.h"
#include "ios/chrome/browser/supervised_user/model/supervised_user_settings_service_factory.h"
#include "ios/chrome/browser/sync/model/data_type_store_service_factory.h"
#include "ios/chrome/browser/sync/model/device_info_sync_service_factory.h"
#include "ios/chrome/browser/sync/model/ios_user_event_service_factory.h"
#include "ios/chrome/browser/sync/model/send_tab_to_self_sync_service_factory.h"
#include "ios/chrome/browser/sync/model/session_sync_service_factory.h"
#include "ios/chrome/browser/sync/model/sync_service_factory.h"
#include "ios/chrome/browser/translate/model/translate_ranker_factory.h"
#include "ios/chrome/browser/unified_consent/model/unified_consent_service_factory.h"
#include "ios/chrome/browser/webauthn/model/ios_passkey_model_factory.h"
#include "ios/chrome/browser/webdata_services/model/web_data_service_factory.h"

#if BUILDFLAG(IOS_CREDENTIAL_PROVIDER_ENABLED)
#import "ios/chrome/browser/credential_provider/model/credential_provider_service_factory.h"
#endif

void EnsureBrowserStateKeyedServiceFactoriesBuilt() {
  autofill::PersonalDataManagerFactory::GetInstance();
  data_sharing::DataSharingServiceFactory::GetInstance();
  ios::AccountBookmarkSyncServiceFactory::GetInstance();
  ios::AccountConsistencyServiceFactory::GetInstance();
  ios::AccountReconcilorFactory::GetInstance();
  ios::BookmarkModelFactory::GetInstance();
  ios::BookmarkUndoServiceFactory::GetInstance();
  ios::FaviconServiceFactory::GetInstance();
  ios::HistoryServiceFactory::GetInstance();
  ios::LocalOrSyncableBookmarkSyncServiceFactory::GetInstance();
  ios::TopSitesFactory::GetInstance();
  ios::AutocompleteClassifierFactory::GetInstance();
  ios::HostContentSettingsMapFactory::GetInstance();
  ios::TemplateURLServiceFactory::GetInstance();
  ios::WebDataServiceFactory::GetInstance();
  ios::WebHistoryServiceFactory::GetInstance();
  ios::ZeroSuggestCacheServiceFactory::GetInstance();
  segmentation_platform::SegmentationPlatformServiceFactory::GetInstance();
  tab_groups::TabGroupSyncServiceFactory::GetInstance();
  translate::TranslateRankerFactory::GetInstance();
  AboutThisSiteServiceFactory::GetInstance();
  BackgroundDownloadServiceFactory::GetInstance();
  BrowserListFactory::GetInstance();
  ChildAccountServiceFactory::GetInstance();
  ConsentAuditorFactory::GetInstance();
  DeviceInfoSyncServiceFactory::GetInstance();
  GoogleGroupsManagerFactory::GetInstance();
  IdentityManagerFactory::GetInstance();
  IOSChromeAccountPasswordStoreFactory::GetInstance();
  IOSChromeFaviconLoaderFactory::GetInstance();
  IOSChromeLargeIconCacheFactory::GetInstance();
  IOSChromeLargeIconServiceFactory::GetInstance();
  IOSChromePasswordReceiverServiceFactory::GetInstance();
  IOSChromePasswordReuseManagerFactory::GetInstance();
  IOSChromePasswordSenderServiceFactory::GetInstance();
  IOSChromeProfilePasswordStoreFactory::GetInstance();
  IOSChromeProfileInvalidationProviderFactory::GetInstance();
  IOSPasskeyModelFactory::GetInstance();
  IOSPasswordManagerSettingsServiceFactory::GetInstance();
  IOSUserEventServiceFactory::GetInstance();
  ListFamilyMembersServiceFactory::GetInstance();
  OptimizationGuideServiceFactory::GetInstance();
  DataTypeStoreServiceFactory::GetInstance();
  PlusAddressServiceFactory::GetInstance();
  PlusAddressSettingServiceFactory::GetInstance();
  PowerBookmarkServiceFactory::GetInstance();
  PushNotificationBrowserStateServiceFactory::GetInstance();
  SupervisedUserServiceFactory::GetInstance();
  SyncServiceFactory::GetInstance();
  UnifiedConsentServiceFactory::GetInstance();
  ReadingListModelFactory::GetInstance();
  SafeBrowsingMetricsCollectorFactory::GetInstance();
  SendTabToSelfSyncServiceFactory::GetInstance();
  SessionRestorationServiceFactory::GetInstance();
  SessionSyncServiceFactory::GetInstance();
  SupervisedUserSettingsServiceFactory::GetInstance();

#if BUILDFLAG(IOS_CREDENTIAL_PROVIDER_ENABLED)
  CredentialProviderServiceFactory::GetInstance();
#endif

  brave::EnsureBrowserStateKeyedServiceFactoriesBuilt();
}
