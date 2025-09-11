/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "base/check.h"
#include "brave/browser/infobars/dev_channel_deprecation_infobar_delegate.h"
#include "brave/browser/ui/startup/brave_obsolete_system_infobar_delegate.h"
#include "build/build_config.h"
#include "chrome/browser/ui/session_crashed_bubble.h"
#include "chrome/browser/ui/startup/google_api_keys_infobar_delegate.h"
#include "components/infobars/content/content_infobar_manager.h"

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC)
#include "chrome/browser/ui/startup/obsolete_system_infobar_delegate.h"
#endif

class BraveGoogleKeysInfoBarDelegate {
 public:
  static void Create(infobars::ContentInfoBarManager* infobar_manager) {
    // lulz
  }
};

#define ShowIfNotOffTheRecordProfile ShowIfNotOffTheRecordProfileBrave
#define GoogleApiKeysInfoBarDelegate BraveGoogleKeysInfoBarDelegate
#define AddInfoBarsIfNecessary AddInfoBarsIfNecessary_ChromiumImpl

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC)
#define ObsoleteSystemInfoBarDelegate BraveObsoleteSystemInfoBarDelegate
#endif

#include <chrome/browser/ui/startup/infobar_utils.cc>

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC)
#undef ObsoleteSystemInfoBarDelegate
#endif

#undef AddInfoBarsIfNecessary
#undef GoogleApiKeysInfoBarDelegate
#undef ShowIfNotOffTheRecordProfile

void AddInfoBarsIfNecessary(BrowserWindowInterface* browser,
                            Profile* profile,
                            const base::CommandLine& startup_command_line,
                            chrome::startup::IsFirstRun is_first_run,
                            bool is_web_app,
                            bool is_post_crash_launch,
                            bool was_restarted) {
  AddInfoBarsIfNecessary_ChromiumImpl(browser, profile, startup_command_line,
                                      is_first_run, is_web_app,
                                      is_post_crash_launch, was_restarted);

  TabStripModel* tab_strip_model = browser->GetTabStripModel();
  if (!browser || !profile || tab_strip_model->count() == 0) {
    return;
  }

  if (IsKioskModeEnabled()) {
    return;
  }

  if (!startup_command_line.HasSwitch(switches::kTestType) &&
      !IsAutomationEnabled()) {
    static bool infobars_shown = false;
    if (infobars_shown) {
      return;
    }
    infobars_shown = true;

    content::WebContents* web_contents =
        tab_strip_model->GetActiveWebContents();
    DCHECK(web_contents);
    infobars::ContentInfoBarManager* infobar_manager =
        infobars::ContentInfoBarManager::FromWebContents(web_contents);
    DevChannelDeprecationInfoBarDelegate::CreateIfNeeded(infobar_manager);
  }
}
