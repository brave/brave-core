// Copyright (c) 2019 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/profiles/brave_profile_manager.h"

#include <memory>
#include <string>
#include <vector>

#include "base/metrics/histogram_macros.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/tor/buildflags.h"
#include "brave/browser/tor/tor_profile_service.h"
#include "brave/browser/tor/tor_profile_service_factory.h"
#include "brave/browser/translate/buildflags/buildflags.h"
#include "brave/common/pref_names.h"
#include "brave/common/tor/pref_names.h"
#include "brave/common/tor/tor_constants.h"
#include "brave/components/brave_ads/browser/ads_service_factory.h"
#include "brave/components/brave_rewards/browser/rewards_service_factory.h"
#include "brave/components/brave_shields/browser/ad_block_regional_service.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/browser/brave_shields_util.h"
#include "brave/components/brave_wallet/browser/buildflags/buildflags.h"
#include "brave/components/brave_webtorrent/browser/buildflags/buildflags.h"
#include "brave/content/browser/webui/brave_shared_resources_data_source.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/chrome_notification_types.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profiles_state.h"
#include "chrome/common/chrome_constants.h"
#include "chrome/common/pref_names.h"
#include "chrome/grit/generated_resources.h"
#include "components/bookmarks/common/bookmark_pref_names.h"
#include "components/gcm_driver/gcm_buildflags.h"
#include "components/prefs/pref_service.h"
#include "components/safe_browsing/core/common/safe_browsing_prefs.h"
#include "components/signin/public/base/signin_pref_names.h"
#include "components/translate/core/browser/translate_pref_names.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/notification_service.h"
#include "content/public/browser/notification_source.h"
#include "content/public/browser/url_data_source.h"
#include "extensions/buildflags/buildflags.h"
#include "third_party/blink/public/common/peerconnection/webrtc_ip_handling_policy.h"
#include "ui/base/l10n/l10n_util.h"

#if BUILDFLAG(ENABLE_EXTENSIONS)
#include "chrome/browser/extensions/extension_service.h"
#include "extensions/browser/extension_system.h"
#endif

#if BUILDFLAG(BRAVE_WALLET_ENABLED)
#include "brave/browser/brave_wallet/brave_wallet_service_factory.h"
#endif

using content::BrowserThread;

#if !BUILDFLAG(USE_GCM_FROM_PLATFORM)
#include "brave/browser/gcm_driver/brave_gcm_channel_status.h"
#endif

BraveProfileManager::BraveProfileManager(const base::FilePath& user_data_dir)
    : ProfileManager(user_data_dir) {
  MigrateProfileNames();

  registrar_.Add(this, chrome::NOTIFICATION_PROFILE_CREATED,
                 content::NotificationService::AllSources());
}

BraveProfileManager::~BraveProfileManager() {
  std::vector<Profile*> profiles = GetLoadedProfiles();
  for (Profile* profile : profiles) {
    if (brave::IsSessionProfile(profile)) {
      // passing false for `success` removes the profile from the info cache
      OnProfileCreated(profile, false, false);
    }
  }
}

// static
// TODO(bridiver) - this should take the last used profile dir as an argument
base::FilePath BraveProfileManager::GetTorProfilePath() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  ProfileManager* profile_manager = g_browser_process->profile_manager();
  base::FilePath parent_path =
      profile_manager->GetLastUsedProfileDir(profile_manager->user_data_dir());

  DCHECK(!brave::IsTorProfilePath(parent_path));

  base::FilePath tor_path = parent_path.AppendASCII("session_profiles");
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
  pref_service->SetString(prefs::kWebRTCIPHandlingPolicy,
                          blink::kWebRTCIPHandlingDisableNonProxiedUdp);
  pref_service->SetBoolean(prefs::kSafeBrowsingEnabled, false);
  // https://blog.torproject.org/bittorrent-over-tor-isnt-good-idea
#if BUILDFLAG(ENABLE_BRAVE_WEBTORRENT)
  pref_service->SetBoolean(kWebTorrentEnabled, false);
#endif
  // Disable the automatic translate bubble in Tor because we currently don't
  // support extensions in Tor mode and users cannot disable this through
  // settings page for Tor windows.
#if BUILDFLAG(ENABLE_BRAVE_TRANSLATE_EXTENSION)
  pref_service->SetBoolean(prefs::kOfferTranslateEnabled, false);
#endif
}

void BraveProfileManager::InitProfileUserPrefs(Profile* profile) {
  if (brave::IsTorProfile(profile)) {
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

void BraveProfileManager::DoFinalInitForServices(Profile* profile,
                                                 bool go_off_the_record) {
  ProfileManager::DoFinalInitForServices(profile, go_off_the_record);
  brave_ads::AdsServiceFactory::GetForProfile(profile);
  brave_rewards::RewardsServiceFactory::GetForProfile(profile);
#if BUILDFLAG(BRAVE_WALLET_ENABLED)
  BraveWalletServiceFactory::GetForProfile(profile);
#endif
#if !BUILDFLAG(USE_GCM_FROM_PLATFORM)
  gcm::BraveGCMChannelStatus::GetForProfile(profile);
#endif
}

void BraveProfileManager::OnProfileCreated(Profile* profile,
                                           bool success,
                                           bool is_new_profile) {
  ProfileManager::OnProfileCreated(profile, success, is_new_profile);

  if (!success)
    return;

#if BUILDFLAG(ENABLE_TOR)
  if (brave::IsTorProfile(profile)) {
    ProfileAttributesEntry* entry;
    ProfileAttributesStorage& storage = GetProfileAttributesStorage();
    if (storage.GetProfileAttributesWithPath(profile->GetPath(), &entry)) {
      profile->GetPrefs()->SetBoolean(prefs::kForceEphemeralProfiles, true);
      entry->SetIsEphemeral(true);
    }

#if BUILDFLAG(ENABLE_EXTENSIONS)
    // This is added in addition to our AreExtensionsDisabled override in
    // brave_extensions_browser_client_impl because there were extension
    // icons briefly showing up when opening Tor windows with that override
    // only.
    extensions::ExtensionService* extension_service =
        extensions::ExtensionSystem::Get(profile)->extension_service();
    extension_service->BlockAllExtensions();
#endif

    // We need to wait until OnProfileCreated to
    // ensure that the request context is available.
    TorProfileServiceFactory::GetForProfile(profile);
  }
#endif
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

void BraveProfileManager::MigrateProfileNames() {
#if !defined(OS_ANDROID) && !defined(OS_CHROMEOS)
  // If any profiles have a default name using an
  // older version of the default name string format,
  // then name it with the new default name string format.
  // e.g. 'Person X' --> 'Profile X'.
  ProfileAttributesStorage& storage = GetProfileAttributesStorage();
  std::vector<ProfileAttributesEntry*> entries =
      storage.GetAllProfilesAttributesSortedByName();
  // Make sure we keep the numbering the same.
  for (auto* entry : entries) {
    // Rename the necessary profiles. Don't check for legacy names as profile
    // info cache should have migrated them by now.
    if (entry->IsUsingDefaultName() &&
        !storage.IsDefaultProfileName(
            entry->GetName(),
            /*include_check_for_legacy_profile_name=*/false)) {
      auto icon_index = entry->GetAvatarIconIndex();
      entry->SetLocalProfileName(storage.ChooseNameForNewProfile(icon_index));
    }
  }
#endif
}

void BraveProfileManager::Observe(int type,
                                  const content::NotificationSource& source,
                                  const content::NotificationDetails& details) {
  switch (type) {
    case chrome::NOTIFICATION_PROFILE_CREATED: {
      Profile* profile = content::Source<Profile>(source).ptr();
      content::URLDataSource::Add(
          profile,
          std::make_unique<brave_content::BraveSharedResourcesDataSource>());
      break;
    }
    default: {
      ProfileManager::Observe(type, source, details);
      break;
    }
  }
}
