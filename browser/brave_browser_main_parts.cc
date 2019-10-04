/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_browser_main_parts.h"

#include "base/command_line.h"
#include "brave/browser/browsing_data/brave_clear_browsing_data.h"
#include "brave/browser/tor/buildflags.h"
#include "brave/common/pref_names.h"
#include "components/prefs/pref_service.h"
#include "content/public/browser/render_frame_host.h"
#include "media/base/media_switches.h"

#if BUILDFLAG(ENABLE_TOR)
#include "base/files/file_util.h"
#include "brave/common/tor/tor_constants.h"
#include "chrome/browser/browser_process_impl.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_metrics.h"
#endif

#if !defined(OS_ANDROID)
#include "brave/browser/infobars/brave_confirm_p3a_infobar_delegate.h"
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
