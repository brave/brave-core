// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ios/chrome/browser/prefs/browser_prefs.h"

#include "brave/components/brave_sync/brave_sync_prefs.h"

#include "components/autofill/core/common/autofill_prefs.h"
#include "components/browsing_data/core/pref_names.h"
#include "components/flags_ui/pref_service_flags_storage.h"
#import "components/handoff/pref_names_ios.h"
#include "components/history/core/common/pref_names.h"
#include "components/invalidation/impl/invalidator_registrar_with_memory.h"
#include "components/invalidation/impl/per_user_topic_subscription_manager.h"
#include "components/language/core/browser/language_prefs.h"
#include "components/language/core/browser/pref_names.h"
#include "components/metrics/metrics_pref_names.h"
#include "components/policy/core/browser/browser_policy_connector.h"
#include "components/policy/core/browser/url_blacklist_manager.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/proxy_config/pref_proxy_config_tracker_impl.h"
#include "components/sessions/core/session_id_generator.h"
#include "components/signin/public/base/signin_pref_names.h"
#include "components/signin/public/identity_manager/identity_manager.h"
#include "components/sync/base/sync_prefs.h"
#include "components/sync_device_info/device_info_prefs.h"
#include "components/sync_sessions/session_sync_prefs.h"
#include "components/unified_consent/unified_consent_service.h"
#include "components/ukm/ios/features.h"
#include "components/variations/service/variations_service.h"
#include "components/web_resource/web_resource_pref_names.h"

#include "ios/chrome/browser/pref_names.h"
#include "ui/base/l10n/l10n_util.h"

#if !defined(__has_feature) || !__has_feature(objc_arc)
#error "This file requires ARC support."
#endif

namespace {
const char kLastKnownGoogleURL[] = "browser.last_known_google_url";
const char kLastPromptedGoogleURL[] = "browser.last_prompted_google_url";

// Deprecated 9/2019
const char kGoogleServicesUsername[] = "google.services.username";
const char kGoogleServicesUserAccountId[] = "google.services.user_account_id";

// Deprecated 1/2020
const char kGCMChannelStatus[] = "gcm.channel_status";
const char kGCMChannelPollIntervalSeconds[] = "gcm.poll_interval";
const char kGCMChannelLastCheckTime[] = "gcm.check_time";

// Deprecated 2/2020
const char kInvalidatorClientId[] = "invalidator.client_id";
const char kInvalidatorInvalidationState[] = "invalidator.invalidation_state";
const char kInvalidatorSavedInvalidations[] = "invalidator.saved_invalidations";
}

//Added by Brandon-T for iOS Sync compiling to reduce dependencies
namespace {
const int IDS_DEFAULT_ENCODING = 14431; //en_US
const int64_t kFolderNone = -1; //from ios/chrome/browser/ui/bookmarks/bookmark_path_cache.mm
const int64_t kLastUsedFolderNone = -1; //from ios/chrome/browser/ui/bookmarks/bookmark_mediator.mm
}

//Added by Brandon-T for iOS Sync compiling to reduce dependencies
namespace prefs {
// Boolean that is true when offering translate (i.e. the automatic translate
// bubble) is enabled. Even when this is false, the user can force translate
// from the right-click context menu unless translate is disabled by policy.
const char kOfferTranslateEnabled[] = "translate.enabled";
}

void RegisterLocalStatePrefs(PrefRegistrySimple* registry) {
  // BrowserStateInfoCache::RegisterPrefs(registry);
  flags_ui::PrefServiceFlagsStorage::RegisterPrefs(registry);
  signin::IdentityManager::RegisterLocalStatePrefs(registry);
  // IOSChromeMetricsServiceClient::RegisterPrefs(registry);
  // network_time::NetworkTimeTracker::RegisterPrefs(registry);
  // ios::NotificationPromo::RegisterPrefs(registry);
  policy::BrowserPolicyConnector::RegisterPrefs(registry);
  // policy::PolicyStatisticsCollector::RegisterPrefs(registry);
  // PrefProxyConfigTrackerImpl::RegisterPrefs(registry);             //Removed because we are registering via the same state for local_state and browser_state for Brave iOS and this is already registered
  // rappor::RapporServiceImpl::RegisterPrefs(registry);
  sessions::SessionIdGenerator::RegisterPrefs(registry);
  // update_client::RegisterPrefs(registry);
  variations::VariationsService::RegisterPrefs(registry);

  // Preferences related to the browser state manager.
  registry->RegisterStringPref(prefs::kBrowserStateLastUsed, std::string());
  registry->RegisterIntegerPref(prefs::kBrowserStatesNumCreated, 1);
  registry->RegisterListPref(prefs::kBrowserStatesLastActive);

  //[OmniboxGeolocationLocalState registerLocalState:registry];
  //[MemoryDebuggerManager registerLocalState:registry];

  registry->RegisterBooleanPref(prefs::kBrowsingDataMigrationHasBeenPossible,
                                false);

  // Preferences related to the application context.
  registry->RegisterStringPref(language::prefs::kApplicationLocale,
                               std::string());
  registry->RegisterBooleanPref(prefs::kEulaAccepted, false);
  registry->RegisterBooleanPref(metrics::prefs::kMetricsReportingEnabled,
                                false);
  registry->RegisterBooleanPref(prefs::kLastSessionExitedCleanly, true);
  if (!base::FeatureList::IsEnabled(kUmaCellular)) {
    registry->RegisterBooleanPref(prefs::kMetricsReportingWifiOnly, true);
  }

    
    //Removed because we are registering via the same state for local_state and browser_state for Brave iOS and this is already registered
//  registry->RegisterBooleanPref(kGCMChannelStatus, true);
//  registry->RegisterIntegerPref(kGCMChannelPollIntervalSeconds, 0);
//  registry->RegisterInt64Pref(kGCMChannelLastCheckTime, 0);
//
//  registry->RegisterListPref(kInvalidatorSavedInvalidations);
//  registry->RegisterStringPref(kInvalidatorInvalidationState, std::string());
//  registry->RegisterStringPref(kInvalidatorClientId, std::string());
}

void RegisterBrowserStatePrefs(user_prefs::PrefRegistrySyncable* registry) {
  autofill::prefs::RegisterProfilePrefs(registry);
  // dom_distiller::DistilledPagePrefs::RegisterProfilePrefs(registry);
  // feed::prefs::RegisterFeedSharedProfilePrefs(registry);
  // FirstRun::RegisterProfilePrefs(registry);
  // FontSizeTabHelper::RegisterBrowserStatePrefs(registry);
  // HostContentSettingsMap::RegisterProfilePrefs(registry);
  // ios::NotificationPromo::RegisterProfilePrefs(registry);
  language::LanguagePrefs::RegisterProfilePrefs(registry);
  // ntp_snippets::ClickBasedCategoryRanker::RegisterProfilePrefs(registry);
  // ntp_snippets::ContentSuggestionsService::RegisterProfilePrefs(registry);
  // ntp_snippets::RemoteSuggestionsProviderImpl::RegisterProfilePrefs(registry);
  // ntp_snippets::RemoteSuggestionsSchedulerImpl::RegisterProfilePrefs(registry);
  // ntp_snippets::RequestThrottler::RegisterProfilePrefs(registry);
  // ntp_snippets::UserClassifier::RegisterProfilePrefs(registry);
  // ntp_tiles::MostVisitedSites::RegisterProfilePrefs(registry);
  // ntp_tiles::PopularSitesImpl::RegisterProfilePrefs(registry);
  // password_manager::PasswordManager::RegisterProfilePrefs(registry);
  // payments::RegisterProfilePrefs(registry);
  policy::URLBlacklistManager::RegisterProfilePrefs(registry);
  PrefProxyConfigTrackerImpl::RegisterProfilePrefs(registry);
  // RegisterVoiceSearchBrowserStatePrefs(registry);
  // safe_browsing::RegisterProfilePrefs(registry);
  sync_sessions::SessionSyncPrefs::RegisterProfilePrefs(registry);
  syncer::DeviceInfoPrefs::RegisterProfilePrefs(registry);
  syncer::InvalidatorRegistrarWithMemory::RegisterProfilePrefs(registry);
  syncer::PerUserTopicSubscriptionManager::RegisterProfilePrefs(registry);
  syncer::SyncPrefs::RegisterProfilePrefs(registry);
  // TemplateURLPrepopulateData::RegisterProfilePrefs(registry);
  // translate::TranslatePrefs::RegisterProfilePrefs(registry);
  unified_consent::UnifiedConsentService::RegisterPrefs(registry);
  //variations::VariationsService::RegisterProfilePrefs(registry);  //Removed because we are registering via the same state for local_state and browser_state for Brave iOS and this is already registered
  // ZeroSuggestProvider::RegisterProfilePrefs(registry);


  //----- BEGIN -----

  //Added by Brandon-T because we can't import the iOS classes mentioned in the comments below, 
  //due to their heavy dependencies on Chrome iOS UI


  //Register bookmarks folders for iOS manually because we can't use [BookmarkMediator registerBrowserStatePrefs:registry];
  registry->RegisterInt64Pref(prefs::kIosBookmarkFolderDefault, kLastUsedFolderNone);

  //Register bookmarks folders for iOS manually because we can't use [BookmarkPathCache registerBrowserStatePrefs:registry];
  registry->RegisterInt64Pref(prefs::kIosBookmarkCachedFolderId, kFolderNone);
  registry->RegisterIntegerPref(prefs::kIosBookmarkCachedTopMostRow, 0);
  
  //Register bookmarks sign-in promos manually because we can't use [SigninPromoViewMediator registerBrowserStatePrefs:registry];
  registry->RegisterBooleanPref(prefs::kIosBookmarkPromoAlreadySeen, false);
  registry->RegisterIntegerPref(prefs::kIosBookmarkSigninPromoDisplayedCount, 0);
  registry->RegisterBooleanPref(prefs::kIosSettingsPromoAlreadySeen, false);
  registry->RegisterIntegerPref(prefs::kIosSettingsSigninPromoDisplayedCount, 0);

  //Register hand-off manually because we can't use [HandoffManager registerBrowserStatePrefs:registry];
  registry->RegisterBooleanPref(
      prefs::kIosHandoffToOtherDevices, true,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);

  //----- END -----

  brave_sync::Prefs::RegisterProfilePrefs(registry);

  registry->RegisterBooleanPref(prefs::kDataSaverEnabled, false);
  registry->RegisterBooleanPref(
      prefs::kEnableDoNotTrack, false,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
  registry->RegisterBooleanPref(
      prefs::kOfferTranslateEnabled, true,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);

  registry->RegisterStringPref(prefs::kDefaultCharset,
                               l10n_util::GetStringUTF8(IDS_DEFAULT_ENCODING),
                               user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
  registry->RegisterBooleanPref(
      prefs::kNetworkPredictionEnabled, true,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
  registry->RegisterBooleanPref(
      prefs::kNetworkPredictionWifiOnly, true,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
  registry->RegisterStringPref(prefs::kContextualSearchEnabled, std::string(),
                               user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
  registry->RegisterBooleanPref(
      prefs::kSearchSuggestEnabled, true,
      user_prefs::PrefRegistrySyncable::SYNCABLE_PREF);
  registry->RegisterBooleanPref(prefs::kSavingBrowserHistoryDisabled, false);

  // This comes from components/bookmarks/core/browser/bookmark_model.h
  // Defaults to 3, which is the id of bookmarkModel_->mobile_node()
  registry->RegisterInt64Pref(prefs::kNtpShownBookmarksFolder, 3);

  // Register prefs used by Clear Browsing Data UI.
  browsing_data::prefs::RegisterBrowserUserPrefs(registry);

  registry->RegisterStringPref(kLastKnownGoogleURL, std::string());
  registry->RegisterStringPref(kLastPromptedGoogleURL, std::string());
  registry->RegisterStringPref(kGoogleServicesUsername, std::string());
  registry->RegisterStringPref(kGoogleServicesUserAccountId, std::string());

  registry->RegisterBooleanPref(kGCMChannelStatus, true);
  registry->RegisterIntegerPref(kGCMChannelPollIntervalSeconds, 0);
  registry->RegisterInt64Pref(kGCMChannelLastCheckTime, 0);

  registry->RegisterListPref(kInvalidatorSavedInvalidations);
  registry->RegisterStringPref(kInvalidatorInvalidationState, std::string());
  registry->RegisterStringPref(kInvalidatorClientId, std::string());
}
