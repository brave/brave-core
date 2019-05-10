// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/profiles/brave_profile_manager.h"

#include <memory>
#include <string>

#include "base/metrics/histogram_macros.h"
#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/extensions/brave_tor_client_updater.h"
#include "brave/browser/tor/tor_profile_service.h"
#include "brave/browser/tor/tor_profile_service_factory.h"
#include "brave/common/tor/pref_names.h"
#include "brave/common/tor/tor_constants.h"
#include "brave/components/brave_ads/browser/ads_service_factory.h"
#include "brave/components/brave_rewards/browser/rewards_service_factory.h"
#include "brave/components/brave_shields/browser/ad_block_regional_service.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_sync/brave_sync_service_factory.h"
#include "brave/content/browser/webui/brave_shared_resources_data_source.h"
#include "chrome/browser/browser_process.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/pref_names.h"
#include "chrome/grit/generated_resources.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "components/prefs/pref_service.h"
#include "components/safe_browsing/common/safe_browsing_prefs.h"
#include "components/signin/core/browser/signin_pref_names.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/url_data_source.h"
#include "content/public/common/webrtc_ip_handling_policy.h"
#include "ui/base/l10n/l10n_util.h"

using content::BrowserThread;

BraveProfileManager::BraveProfileManager(const base::FilePath& user_data_dir)
  : ProfileManager(user_data_dir) {}

// static
base::FilePath BraveProfileManager::GetTorProfilePath() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  ProfileManager* profile_manager = g_browser_process->profile_manager();

  base::FilePath tor_path = profile_manager->user_data_dir();
  return tor_path.Append(tor::kTorProfileDir);
}

// static
void BraveProfileManager::InitTorProfileUserPrefs(Profile* profile) {
  PrefService* pref_service = profile->GetPrefs();
  pref_service->SetInteger(prefs::kProfileAvatarIndex, 0);
  pref_service->SetBoolean(prefs::kProfileUsingDefaultName, false);
  pref_service
    ->SetString(prefs::kProfileName,
                l10n_util::GetStringUTF8(IDS_PROFILES_TOR_PROFILE_NAME));
  pref_service->SetBoolean(tor::prefs::kProfileUsingTor, true);
  pref_service->SetString(prefs::kWebRTCIPHandlingPolicy,
                          content::kWebRTCIPHandlingDisableNonProxiedUdp);
  pref_service->SetBoolean(prefs::kSafeBrowsingEnabled, false);
}

void BraveProfileManager::InitProfileUserPrefs(Profile* profile) {
  if (profile->GetPath() == GetTorProfilePath()) {
    InitTorProfileUserPrefs(profile);
  } else {
    ProfileManager::InitProfileUserPrefs(profile);
  }
}

std::string BraveProfileManager::GetLastUsedProfileName() {
  PrefService* local_state = g_browser_process->local_state();
  DCHECK(local_state);
  const std::string last_used_profile_name =
      local_state->GetString(prefs::kProfileLastUsed);
  if (last_used_profile_name ==
          base::FilePath(tor::kTorProfileDir).AsUTF8Unsafe())
    return chrome::kInitialProfile;
  return ProfileManager::GetLastUsedProfileName();
}

Profile* BraveProfileManager::CreateProfileHelper(const base::FilePath& path) {
  TRACE_EVENT0("browser", "ProfileManager::CreateProfileHelper");
  SCOPED_UMA_HISTOGRAM_TIMER("Profile.CreateProfileHelperTime");
  Profile* profile = ProfileManager::CreateProfileHelper(path);
  if (path == GetTorProfilePath()) {
     LaunchTorProcess(profile);
  }
  return profile;
}

Profile*
BraveProfileManager::CreateProfileAsyncHelper(const base::FilePath& path,
                                              Delegate* delegate) {
  Profile* profile = ProfileManager::CreateProfileAsyncHelper(path, delegate);
  if (path == GetTorProfilePath()) {
     LaunchTorProcess(profile);
  }
  return profile;
}

void BraveProfileManager::DoFinalInitForServices(Profile* profile,
                                                 bool go_off_the_record) {
  ProfileManager::DoFinalInitForServices(profile, go_off_the_record);
  // BraveSyncService need to be created when profile initialized, otherwise
  // it will only be constructed only when we open chrome:/sync/
  brave_sync::BraveSyncServiceFactory::GetForProfile(profile);
  brave_ads::AdsServiceFactory::GetForProfile(profile);
  brave_rewards::RewardsServiceFactory::GetForProfile(profile);
  content::URLDataSource::Add(profile,
      std::make_unique<brave_content::BraveSharedResourcesDataSource>());
}

void BraveProfileManager::LaunchTorProcess(Profile* profile) {
  tor::TorProfileService* tor_profile_service =
    TorProfileServiceFactory::GetForProfile(profile);
  if (tor_profile_service->GetTorPid() < 0) {
    base::FilePath path =
      g_brave_browser_process->tor_client_updater()->GetExecutablePath();
    std::string proxy =
      g_browser_process->local_state()->GetString(tor::prefs::kTorProxyString);
    tor::TorConfig config(path, proxy);
    tor_profile_service->LaunchTor(config);
  }
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
