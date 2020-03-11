/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_browser_main_parts.h"

#include "base/command_line.h"
#include "brave/browser/browsing_data/brave_clear_browsing_data.h"
#include "brave/browser/tor/buildflags.h"
#include "brave/common/pref_names.h"
#include "brave/components/brave_sync/features.h"
#include "chrome/common/chrome_features.h"
#include "components/prefs/pref_service.h"
#include "components/sync/driver/sync_driver_switches.h"
#include "content/public/browser/render_frame_host.h"
#include "media/base/media_switches.h"

#if BUILDFLAG(ENABLE_TOR)
#include <string>
#include "base/files/file_util.h"
#include "brave/common/tor/tor_constants.h"
#include "chrome/browser/browser_process_impl.h"
#include "chrome/browser/profiles/profile_attributes_storage.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_metrics.h"
#include "components/account_id/account_id.h"
#endif

#if !defined(OS_ANDROID)
#include "brave/browser/infobars/brave_confirm_p3a_infobar_delegate.h"
#include "brave/browser/infobars/crypto_wallets_infobar_delegate.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/infobars/infobar_service.h"
#include "content/public/browser/web_contents.h"
#endif

#if BUILDFLAG(ENABLE_TOR) || !defined(OS_ANDROID)
#include "chrome/browser/browser_process.h"
#endif

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
    ProfileAttributesEntry* entry = nullptr;
    ProfileAttributesStorage& storage =
        profile_manager->GetProfileAttributesStorage();
    if (!storage.GetProfileAttributesWithPath(tor_legacy_path, &entry)) {
      storage.AddProfile(tor_legacy_path, base::string16(), std::string(),
                         base::string16(),
                         /* is_consented_primary_account*/ false, 0,
                         std::string(), EmptyAccountId());
    }

    profile_manager->MaybeScheduleProfileForDeletion(
        tor_legacy_path, base::DoNothing(),
        ProfileMetrics::DELETE_PROFILE_SETTINGS);
  }
#endif

#if !defined(OS_ANDROID)
  Browser* browser = chrome::FindLastActive();
  content::WebContents* active_web_contents = nullptr;

  if (browser) {
    active_web_contents = browser->tab_strip_model()->GetActiveWebContents();

    if (active_web_contents) {
      InfoBarService* infobar_service =
          InfoBarService::FromWebContents(active_web_contents);

      if (infobar_service) {
        BraveConfirmP3AInfoBarDelegate::Create(
            infobar_service, g_browser_process->local_state());
      }
    }
  }
#endif  // !defined(OS_ANDROID)
}

void BraveBrowserMainParts::PreShutdown() {
  content::BraveClearBrowsingData::ClearOnExit();
}

void BraveBrowserMainParts::PreProfileInit() {
  ChromeBrowserMainParts::PreProfileInit();
#if !defined(OS_ANDROID)
  auto* command_line = base::CommandLine::ForCurrentProcess();
  if (!base::FeatureList::IsEnabled(brave_sync::features::kBraveSync)) {
    // Disable sync temporarily
    if (!command_line->HasSwitch(switches::kDisableSync))
        command_line->AppendSwitch(switches::kDisableSync);
  } else {
    // Relaunch after flag changes will still have the switch
    // when switching from disabled to enabled
    command_line->RemoveSwitch(switches::kDisableSync);
  }
#endif
}

void BraveBrowserMainParts::PostProfileInit() {
  ChromeBrowserMainParts::PostProfileInit();

#if defined(OS_ANDROID)
  if (profile()->GetPrefs()->GetBoolean(kBackgroundVideoPlaybackEnabled)) {
    content::RenderFrameHost::AllowInjectingJavaScript();
    auto* command_line = base::CommandLine::ForCurrentProcess();
    command_line->AppendSwitch(switches::kDisableMediaSuspend);
  }
#endif
}
