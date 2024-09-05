/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_browser_main_parts.h"

#include <memory>
#include <utility>

#include "base/command_line.h"
#include "brave/browser/browsing_data/brave_clear_browsing_data.h"
#include "brave/browser/ethereum_remote_client/buildflags/buildflags.h"
#include "brave/components/brave_component_updater/browser/brave_on_demand_updater.h"
#include "brave/components/brave_rewards/common/rewards_flags.h"
#include "brave/components/brave_rewards/common/rewards_util.h"
#include "brave/components/brave_sync/features.h"
#include "brave/components/constants/brave_constants.h"
#include "brave/components/constants/pref_names.h"
#include "brave/components/speedreader/common/buildflags/buildflags.h"
#include "brave/components/tor/buildflags/buildflags.h"
#include "build/build_config.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/chrome_features.h"
#include "components/component_updater/component_updater_service.h"
#include "components/prefs/pref_service.h"
#include "components/sync/base/command_line_switches.h"
#include "content/public/browser/render_frame_host.h"
#include "extensions/buildflags/buildflags.h"
#include "media/base/media_switches.h"

#if BUILDFLAG(ENABLE_SPEEDREADER)
#include "brave/components/speedreader/speedreader_extended_info_handler.h"
#include "components/sessions/content/content_serialized_navigation_driver.h"
#endif

#if BUILDFLAG(ENABLE_TOR)
#include <string>
#include "base/files/file_util.h"
#include "brave/components/tor/tor_constants.h"
#include "chrome/browser/browser_process_impl.h"
#include "chrome/browser/profiles/profile_attributes_init_params.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_metrics.h"
#include "components/account_id/account_id.h"
#endif

#if !BUILDFLAG(IS_ANDROID)
#include "brave/browser/infobars/brave_confirm_p3a_infobar_delegate.h"
#include "brave/browser/infobars/brave_sync_account_deleted_infobar_delegate.h"
#include "brave/browser/infobars/sync_cannot_run_infobar_delegate.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_finder.h"
#include "components/infobars/content/content_infobar_manager.h"
#include "components/sync/service/sync_service.h"
#include "components/sync/service/sync_user_settings.h"
#include "content/public/browser/web_contents.h"
#else
#include "brave/browser/android/preferences/features.h"
#endif

#if BUILDFLAG(ETHEREUM_REMOTE_CLIENT_ENABLED) && BUILDFLAG(ENABLE_EXTENSIONS)
#include "brave/browser/extensions/brave_component_loader.h"
#include "chrome/browser/extensions/extension_service.h"
#include "extensions/browser/extension_system.h"
#endif

#if !BUILDFLAG(IS_ANDROID)
#include "base/path_service.h"
#include "brave/components/ipfs/ipfs_component_cleaner.h"
#include "chrome/common/chrome_paths.h"
#endif  // !BUILDFLAG(IS_ANDROID)

BraveBrowserMainParts::BraveBrowserMainParts(bool is_integration_test,
                                             StartupData* startup_data)
    : ChromeBrowserMainParts(is_integration_test, startup_data) {}

BraveBrowserMainParts::~BraveBrowserMainParts() = default;

int BraveBrowserMainParts::PreMainMessageLoopRun() {
  brave_component_updater::BraveOnDemandUpdater::GetInstance()
      ->RegisterOnDemandUpdater(
          &g_browser_process->component_updater()->GetOnDemandUpdater());

  return ChromeBrowserMainParts::PreMainMessageLoopRun();
}

void BraveBrowserMainParts::PreBrowserStart() {
#if BUILDFLAG(ENABLE_SPEEDREADER)
  // Register() must be called after the SerializedNavigationDriver is
  // initialized, but before any calls to
  // ContentSerializedNavigationBuilder::ToNavigationEntries()
  //
  // TODO(keur): Can we DCHECK the latter condition?
  DCHECK(sessions::ContentSerializedNavigationDriver::GetInstance());
  speedreader::SpeedreaderExtendedInfoHandler::Register();
#endif

  ChromeBrowserMainParts::PreBrowserStart();
}

void BraveBrowserMainParts::PostBrowserStart() {
  ChromeBrowserMainParts::PostBrowserStart();

#if BUILDFLAG(ENABLE_TOR)
  ProfileManager* profile_manager = g_browser_process->profile_manager();
  base::FilePath tor_legacy_path =
      profile_manager->user_data_dir().Append(tor::kTorProfileDir);

  // Delete Tor legacy profile if exists.
  if (base::PathExists(tor_legacy_path)) {
    // Add tor legacy path into profile attributes storage first if nonexist
    // because we will hit DCHECK(!GetProfileAttributesWithPath(...))  in
    // ProfileInfoCache::DeleteProfileFromCache when we trying to delete it
    // without this being added into the storage first.
    ProfileAttributesStorage& storage =
        profile_manager->GetProfileAttributesStorage();
    ProfileAttributesEntry* entry =
        storage.GetProfileAttributesWithPath(tor_legacy_path);
    if (!entry) {
      ProfileAttributesInitParams params;
      params.profile_path = tor_legacy_path;
      storage.AddProfile(std::move(params));
    }

    profile_manager->GetDeleteProfileHelper().MaybeScheduleProfileForDeletion(
        tor_legacy_path, base::DoNothing(),
        ProfileMetrics::DELETE_PROFILE_SETTINGS);
  }
  for (Profile* profile : profile_manager->GetLoadedProfiles()) {
    const base::FilePath tor_legacy_session_path =
        profile->GetPath()
            .Append(brave::kSessionProfileDir)
            .Append(tor::kTorProfileDir);
    if (base::PathExists(tor_legacy_session_path)) {
      profile_manager->GetDeleteProfileHelper().MaybeScheduleProfileForDeletion(
          tor_legacy_session_path, base::DoNothing(),
          ProfileMetrics::DELETE_PROFILE_SETTINGS);
    }
  }
#endif

#if !BUILDFLAG(IS_ANDROID)
  Browser* browser = chrome::FindLastActive();
  content::WebContents* active_web_contents = nullptr;

  if (browser) {
    active_web_contents = browser->tab_strip_model()->GetActiveWebContents();

    if (active_web_contents) {
      Profile* profile =
          Profile::FromBrowserContext(active_web_contents->GetBrowserContext());
      infobars::ContentInfoBarManager* infobar_manager =
          infobars::ContentInfoBarManager::FromWebContents(active_web_contents);
      if (profile && infobar_manager) {
        BraveConfirmP3AInfoBarDelegate::Create(
            infobar_manager, g_browser_process->local_state());
        SyncCannotRunInfoBarDelegate::Create(infobar_manager, profile, browser);

        BraveSyncAccountDeletedInfoBarDelegate::Create(active_web_contents,
                                                       profile, browser);
      }
    }
  }
#endif  // !BUILDFLAG(IS_ANDROID)

#if !BUILDFLAG(IS_ANDROID)
  ipfs::CleanupIpfsComponent(
      base::PathService::CheckedGet(chrome::DIR_USER_DATA));
#endif  // !BUILDFLAG(IS_ANDROID)
}

void BraveBrowserMainParts::PreShutdown() {
  content::BraveClearBrowsingData::ClearOnExit();
}

void BraveBrowserMainParts::PreProfileInit() {
  ChromeBrowserMainParts::PreProfileInit();
#if !BUILDFLAG(IS_ANDROID)
  auto* command_line = base::CommandLine::ForCurrentProcess();
  if (!base::FeatureList::IsEnabled(brave_sync::features::kBraveSync)) {
    // Disable sync temporarily
    if (!command_line->HasSwitch(syncer::kDisableSync))
      command_line->AppendSwitch(syncer::kDisableSync);
  } else {
    // Relaunch after flag changes will still have the switch
    // when switching from disabled to enabled
    command_line->RemoveSwitch(syncer::kDisableSync);
  }
#endif
}

void BraveBrowserMainParts::PostProfileInit(Profile* profile,
                                            bool is_initial_profile) {
  ChromeBrowserMainParts::PostProfileInit(profile, is_initial_profile);

#if BUILDFLAG(IS_ANDROID)
  if (base::FeatureList::IsEnabled(
          preferences::features::kBraveBackgroundVideoPlayback) ||
      profile->GetPrefs()->GetBoolean(kBackgroundVideoPlaybackEnabled)) {
    content::RenderFrameHost::AllowInjectingJavaScript();
    auto* command_line = base::CommandLine::ForCurrentProcess();
    command_line->AppendSwitch(switches::kDisableBackgroundMediaSuspend);
  }
#endif

#if BUILDFLAG(ETHEREUM_REMOTE_CLIENT_ENABLED) && BUILDFLAG(ENABLE_EXTENSIONS)
  extensions::ExtensionService* service =
      extensions::ExtensionSystem::Get(profile)->extension_service();
  if (service) {
    extensions::ComponentLoader* loader = service->component_loader();
    static_cast<extensions::BraveComponentLoader*>(loader)
        ->AddEthereumRemoteClientExtensionOnStartup();
  }
#endif
}
