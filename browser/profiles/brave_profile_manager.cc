// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/profiles/brave_profile_manager.h"

#include <string>
#include <utility>
#include <vector>

#include "base/check.h"
#include "base/path_service.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/browser/brave_rewards/rewards_service_factory.h"
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#include "brave/browser/misc_metrics/profile_misc_metrics_service_factory.h"
#include "brave/browser/perf/brave_perf_features_processor.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/request_otr/request_otr_service_factory.h"
#include "brave/browser/url_sanitizer/url_sanitizer_service_factory.h"
#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/ai_chat/core/common/features.h"
#include "brave/components/brave_shields/content/browser/brave_shields_util.h"
#include "brave/components/brave_shields/core/browser/brave_shields_p3a.h"
#include "brave/components/brave_shields/core/browser/brave_shields_utils.h"
#include "brave/components/constants/brave_constants.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/content_settings/core/browser/brave_content_settings_pref_provider.h"
#include "brave/components/ntp_background_images/browser/ntp_p3a_util.h"
#include "brave/components/ntp_background_images/common/pref_names.h"
#include "brave/components/request_otr/common/buildflags/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/content_settings/host_content_settings_map_factory.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profiles_state.h"
#include "chrome/common/chrome_paths.h"
#include "chrome/common/pref_names.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "components/gcm_driver/gcm_buildflags.h"
#include "components/prefs/pref_service.h"
#include "components/signin/public/base/signin_pref_names.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/features.h"

#if !BUILDFLAG(USE_GCM_FROM_PLATFORM)
#include "brave/browser/gcm_driver/brave_gcm_channel_status.h"
#endif

#if BUILDFLAG(ENABLE_TOR)
#include "brave/components/tor/tor_constants.h"
#endif

using brave_shields::ControlType;
using content::BrowserThread;
using ntp_background_images::prefs::kNewTabPageShowBackgroundImage;
using ntp_background_images::prefs::
    kNewTabPageShowSponsoredImagesBackgroundImage;  // NOLINT

namespace {

#if !BUILDFLAG(IS_ANDROID)
// WIP to pass over CHECK(!IdentityManagerFactory::GetForProfileIfExists(this))
// on Android tests

// Checks if the user previously had HTTPS-Only Mode enabled. If so,
// set the HttpsUpgrade default setting to strict.
void MigrateHttpsUpgradeSettings(Profile* profile) {
  // If user flips the HTTPS by Default feature flag
  auto* prefs = profile->GetPrefs();
  // The HostContentSettingsMap might be null for some irregular profiles, e.g.
  // the System Profile.
  auto* map = HostContentSettingsMapFactory::GetForProfile(profile);
  if (!map) {
    return;
  }
  if (base::FeatureList::IsEnabled(net::features::kBraveHttpsByDefault)) {
    // Migrate forwards from HTTPS-Only Mode to HTTPS Upgrade Strict setting.
    if (prefs->GetBoolean(prefs::kHttpsOnlyModeEnabled)) {
      brave_shields::SetHttpsUpgradeControlType(map, ControlType::BLOCK,
                                                GURL());
      prefs->SetBoolean(prefs::kHttpsOnlyModeEnabled, false);
    }
  } else {
    // Migrate backwards from HTTPS Upgrade Strict setting to HTTPS-Only Mode.
    if (brave_shields::GetHttpsUpgradeControlType(map, GURL()) ==
        ControlType::BLOCK) {
      prefs->SetBoolean(prefs::kHttpsOnlyModeEnabled, true);
      brave_shields::SetHttpsUpgradeControlType(
          map, ControlType::BLOCK_THIRD_PARTY, GURL());
    }
  }
}
#endif

#if !BUILDFLAG(IS_ANDROID)
// WIP to pass over CHECK(!IdentityManagerFactory::GetForProfileIfExists(this))
// on Android tests
void RecordInitialP3AValues(Profile* profile) {
  // Preference is unregistered for some reason in profile_manager_unittest
  // TODO(bsclifton): create a proper testing profile
  if (!profile->GetPrefs()->FindPreference(kNewTabPageShowBackgroundImage) ||
      !profile->GetPrefs()->FindPreference(
          kNewTabPageShowSponsoredImagesBackgroundImage)) {
    return;
  }
  ntp_background_images::RecordSponsoredImagesEnabledP3A(profile->GetPrefs());
  if (profile->IsRegularProfile()) {
    auto* map = HostContentSettingsMapFactory::GetForProfile(profile);
    MaybeRecordInitialShieldsSettings(
        profile->GetPrefs(), map,
        brave_shields::GetCosmeticFilteringControlType(map, GURL()),
        brave_shields::GetFingerprintingControlType(map, GURL()));
  }
}
#endif

}  // namespace

BraveProfileManager::BraveProfileManager(const base::FilePath& user_data_dir)
    : ProfileManager(user_data_dir) {}

size_t BraveProfileManager::GetNumberOfProfiles() {
  size_t count = ProfileManager::GetNumberOfProfiles();
#if BUILDFLAG(ENABLE_BRAVE_AI_CHAT_AGENT_PROFILE)
  if (ai_chat::features::IsAIChatAgentProfileEnabled()) {
    // Don't include AI Chat agent profile in the count
    base::FilePath ai_chat_agent_profile_path =
        user_data_dir_.Append(brave::kAIChatAgentProfileDir);
    if (count > 0 && GetProfileAttributesStorage().GetProfileAttributesWithPath(
                         ai_chat_agent_profile_path)) {
      count--;
    }
  }
#endif

  return count;
}

void BraveProfileManager::InitProfileUserPrefs(Profile* profile) {
  // migrate obsolete plugin prefs to temporary migration pref because otherwise
  // they get deleteed by PrefProvider before we can migrate them in
  // BravePrefProvider
  content_settings::BravePrefProvider::CopyPluginSettingsForMigration(
      profile->GetPrefs());

// Chromecast is enabled by default on Android.
#if !BUILDFLAG(IS_ANDROID)
  auto* pref_service = profile->GetPrefs();
  // At start, the value of kEnableMediaRouterOnRestart is updated to match
  // kEnableMediaRouter so users don't lose their current setting
  if (pref_service->FindPreference(kEnableMediaRouterOnRestart)
          ->IsDefaultValue()) {
    auto enabled = pref_service->GetBoolean(::prefs::kEnableMediaRouter);
    pref_service->SetBoolean(kEnableMediaRouterOnRestart, enabled);
  } else {
    // For Desktop, kEnableMediaRouterOnRestart is used to track the current
    // state of the media router switch in brave://settings/extensions. The
    // value of kEnableMediaRouter is only updated to match
    // kEnableMediaRouterOnRestart on restart
    auto enabled = pref_service->GetBoolean(kEnableMediaRouterOnRestart);
    pref_service->SetBoolean(::prefs::kEnableMediaRouter, enabled);
  }
#endif

  ProfileManager::InitProfileUserPrefs(profile);
#if !BUILDFLAG(IS_ANDROID)
  // WIP
  // At `RecordInitialP3AValues` the instance of `HostContentSettingsMap` is
  // created:
  // ```
  // HostContentSettingsMapFactory::GetForProfile
  // (anonymous namespace)::RecordInitialP3AValues
  // BraveProfileManager::InitProfileUserPrefs
  // ProfileImpl::OnLocaleReady
  // ProfileImpl::OnPrefsLoaded
  // ProfileImpl::ProfileImpl
  // Profile::CreateProfile
  // ProfileManager::CreateProfileHelper
  // ```
  // At `HostContentSettingsMapFactory::BuildServiceInstanceFor` there is
  // `#if BUILDFLAG(IS_ANDROID)` block which creates `TemplateURLService`, which
  // in its turn creates `IdentityManager`:
  // ```
  // IdentityManagerFactory::BuildServiceInstanceForBrowserContext
  // KeyedServiceTemplatedFactory<KeyedService>::GetServiceForContext
  // search_engines::(anonymous namespace)::BuildSearchEngineChoiceService
  // KeyedServiceTemplatedFactory<KeyedService>::GetServiceForContext
  // TemplateURLServiceFactory::BuildInstanceFor
  // ```
  // We must not create `IdentityManager` at early phase of browser
  // initialization, because there is
  // ```
  //   // Check that the IdentityManager was not created before the browser
  //   context
  //   // services were created. This ensures that browser tests can override
  //   the
  //   // IdentityManager with a fake.
  //   CHECK(!IdentityManagerFactory::GetForProfileIfExists(this),
  //         base::NotFatalUntil::M160);
  // ```
  // at `ProfileImpl::OnPrefsLoaded` since `cr144`.
  // Prior to `cr144` this CHECK was under
  // `kDelayOnProfileCreatedForFullBrowserTransition` flag.
  //
  // Chromium commit:
  // https://source.chromium.org/chromium/chromium/src/+/c2150016298cc880896bc1f81335be279ceac961
  //
  // Skip send P3A here for Android, otherwise most of the tests fail on Android
  RecordInitialP3AValues(profile);
#endif

  brave::SetDefaultSearchVersion(profile, profile->IsNewProfile());
  brave::SetDefaultThirdPartyCookieBlockValue(profile);
  perf::MaybeEnableBraveFeaturesPrefsForPerfTesting(profile);
#if !BUILDFLAG(IS_ANDROID)
  // WIP to pass over
  // CHECK(!IdentityManagerFactory::GetForProfileIfExists(this)) on Android
  // tests
  MigrateHttpsUpgradeSettings(profile);
#endif
}

void BraveProfileManager::DoFinalInitForServices(Profile* profile,
                                                 bool go_off_the_record) {
  ProfileManager::DoFinalInitForServices(profile, go_off_the_record);
  if (!do_final_services_init_) {
    return;
  }
  perf::MaybeEnableBraveFeaturesServicesAndComponentsForPerfTesting(profile);
  brave_ads::AdsServiceFactory::GetForProfile(profile);
  brave_rewards::RewardsServiceFactory::GetForProfile(profile);
  brave_wallet::BraveWalletServiceFactory::GetServiceForContext(profile);
#if !BUILDFLAG(USE_GCM_FROM_PLATFORM)
  gcm::BraveGCMChannelStatus* status =
      gcm::BraveGCMChannelStatus::GetForProfile(profile);
  DCHECK(status);
  status->UpdateGCMDriverStatus();
#endif
  brave::URLSanitizerServiceFactory::GetForBrowserContext(profile);
  misc_metrics::ProfileMiscMetricsServiceFactory::GetServiceForContext(profile);
#if BUILDFLAG(ENABLE_REQUEST_OTR)
  request_otr::RequestOTRServiceFactory::GetForBrowserContext(profile);
#endif
}

bool BraveProfileManager::IsAllowedProfilePath(
    const base::FilePath& path) const {
  // Chromium only allows profiles to be created in the user_data_dir, but we
  // want to also be able to create profile in subfolders of user_data_dir.
  return ProfileManager::IsAllowedProfilePath(path) ||
         user_data_dir().IsParent(path.DirName());
}

bool BraveProfileManager::LoadProfileByPath(const base::FilePath& profile_path,
                                            bool incognito,
                                            ProfileLoadedCallback callback) {
#if BUILDFLAG(ENABLE_TOR)
  // Prevent legacy tor session profile to be loaded so we won't hit
  // DCHECK(!GetProfileAttributesWithPath(...)). Workaround for legacy tor guest
  // profile won't work because when AddProfile to storage we will hit
  // DCHECK(user_data_dir_ == profile_path.DirName()), legacy tor session
  // profile was not under user_data_dir like legacy tor guest profile did.
  if (profile_path.BaseName().value() == tor::kTorProfileDir) {
    return false;
  }
#endif
  return ProfileManager::LoadProfileByPath(profile_path, incognito,
                                           std::move(callback));
}

void BraveProfileManager::SetProfileAsLastUsed(Profile* last_active) {
  // Prevent AI Chat agent profile from having an active time so that
  // `ProfilePicker::GetStartupModeReason` doesn't consider it as a
  // recently-active profile. This change causes it not to be considered
  // for showing the profile picker on startup.
#if BUILDFLAG(ENABLE_BRAVE_AI_CHAT_AGENT_PROFILE)
  if (last_active->IsAIChatAgent()) {
    return;
  }
#endif
  ProfileManager::SetProfileAsLastUsed(last_active);
}

// This overridden method doesn't clear |kDefaultSearchProviderDataPrefName|.
// W/o this, prefs set by TorWindowSearchEngineProviderService is cleared
// during the initialization.
void BraveProfileManager::SetNonPersonalProfilePrefs(Profile* profile) {
  PrefService* prefs = profile->GetPrefs();
  prefs->SetBoolean(prefs::kSigninAllowed, false);
  prefs->SetBoolean(bookmarks::prefs::kEditBookmarksEnabled, false);
  prefs->SetBoolean(bookmarks::prefs::kShowBookmarkBar, false);
}

BraveProfileManagerWithoutInit::BraveProfileManagerWithoutInit(
    const base::FilePath& user_data_dir)
    : BraveProfileManager(user_data_dir) {
  set_do_final_services_init(false);
}
