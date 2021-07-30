/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/startup/default_brave_browser_prompt.h"

#include <string>

#include "base/bind.h"
#include "base/logging.h"
#include "base/version.h"
#include "brave/browser/ui/browser_dialogs.h"
#include "brave/common/pref_names.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/first_run/first_run.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/shell_integration.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_list.h"
#include "chrome/browser/ui/startup/default_browser_prompt.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"
#include "components/prefs/pref_service.h"
#include "components/version_info/version_info.h"
#include "content/public/browser/web_contents.h"

// Many code in this file is copied from default_browser_prompt.cc
namespace {

void IncreaseBrowserLaunchCount(Profile* profile) {
  const int current_count =
      profile->GetPrefs()->GetInteger(kDefaultBrowserLaunchingCount);

  // Don't need to record more because we don't show prompt after 20th launch.
  if (current_count >= 20)
    return;

  profile->GetPrefs()->SetInteger(kDefaultBrowserLaunchingCount,
                                  current_count + 1);
}

void ShowPrompt() {
  // Show the default browser request prompt in the most recently active,
  // visible, tabbed browser. Do not show the prompt if no such browser exists.
  BrowserList* browser_list = BrowserList::GetInstance();
  for (auto browser_iterator = browser_list->begin_last_active();
       browser_iterator != browser_list->end_last_active();
       ++browser_iterator) {
    Browser* browser = *browser_iterator;

    // |browser| may be null in UI tests. Also, don't show the prompt in an app
    // window, which is not meant to be treated as a Chrome window.
    if (!browser || browser->deprecated_is_app())
      continue;

    // In ChromeBot tests, there might be a race. This line appears to get
    // called during shutdown and the active web contents can be nullptr.
    content::WebContents* web_contents =
        browser->tab_strip_model()->GetActiveWebContents();
    if (!web_contents ||
        web_contents->GetVisibility() != content::Visibility::VISIBLE) {
      continue;
    }

    // Never show the default browser prompt over the first run promos.
    // TODO(pmonette): The whole logic that determines when to show the default
    // browser prompt is due for a refactor. ShouldShowDefaultBrowserPrompt()
    // should be aware of the first run promos and return false instead of
    // counting on the early return here. See bug crbug.com/693292.
    if (first_run::IsOnWelcomePage(web_contents))
      continue;

    // Launch dialog.
    brave::ShowDefaultBrowserDialog(browser);
    break;
  }
}

void ResetCheckDefaultBrowserPref(const base::FilePath& profile_path) {
  Profile* profile =
      g_browser_process->profile_manager()->GetProfileByPath(profile_path);
  if (profile)
    ResetDefaultBraveBrowserPrompt(profile);
}

void OnCheckIsDefaultBrowserFinished(
    const base::FilePath& profile_path,
    bool show_prompt,
    shell_integration::DefaultWebClientState state) {
  if (state == shell_integration::IS_DEFAULT ||
      state == shell_integration::OTHER_MODE_IS_DEFAULT) {
    // Notify the user in the future if Chrome ceases to be the user's chosen
    // default browser.
    ResetCheckDefaultBrowserPref(profile_path);
  } else if (show_prompt && state == shell_integration::NOT_DEFAULT &&
             shell_integration::CanSetAsDefaultBrowser()) {
    // Only show the prompt if some other program is the user's default browser.
    // In particular, don't show it if another install mode is default (e.g.,
    // don't prompt for Chrome Beta if stable Chrome is the default).
    ShowPrompt();
  }
}

// Returns true if the default browser prompt should be shown if Chrome is not
// the user's default browser.
bool ShouldShowDefaultBrowserPrompt(Profile* profile) {
  // Do not show the prompt if "suppress_default_browser_prompt_for_version" in
  // the initial preferences is set to the current version.
  const std::string disable_version_string =
      g_browser_process->local_state()->GetString(
          prefs::kBrowserSuppressDefaultBrowserPrompt);
  const base::Version disable_version(disable_version_string);
  DCHECK(disable_version_string.empty() || disable_version.IsValid());
  if (disable_version.IsValid() &&
      disable_version == version_info::GetVersion()) {
    return false;
  }

  const int current_count =
      profile->GetPrefs()->GetInteger(kDefaultBrowserLaunchingCount);

  // We only show prompt at 3rd and 20th.
  // This is not called at first run. So, count 1 is second run.
  if (current_count == 2 || current_count == 19)
    return true;

  return false;
}

}  // namespace

void ShowDefaultBraveBrowserPrompt(Profile* profile) {
#if !defined(OFFICIAL_BUILD)
  // Disable in developer build. Showing with infobar didn't bother much but
  // modal dialog could distract developers.
  return;
#endif

  PrefService* local_prefs = g_browser_process->local_state();
  // Do not check if Chrome is the default browser if there is a policy in
  // control of this setting.
  if (local_prefs->IsManagedPreference(prefs::kDefaultBrowserSettingEnabled)) {
    // Handling of the browser.default_browser_setting_enabled policy setting is
    // taken care of in BrowserProcessImpl.
    return;
  }

  if (!local_prefs->GetBoolean(kDefaultBrowserPromptEnabled))
    return;

  PrefService* prefs = profile->GetPrefs();
  // Reset preferences if kResetCheckDefaultBrowser is true.
  if (prefs->GetBoolean(prefs::kResetCheckDefaultBrowser)) {
    prefs->SetBoolean(prefs::kResetCheckDefaultBrowser, false);
    ResetDefaultBraveBrowserPrompt(profile);
  }

  IncreaseBrowserLaunchCount(profile);

  scoped_refptr<shell_integration::DefaultBrowserWorker>(
      new shell_integration::DefaultBrowserWorker())
      ->StartCheckIsDefault(
          base::BindOnce(&OnCheckIsDefaultBrowserFinished, profile->GetPath(),
                         ShouldShowDefaultBrowserPrompt(profile)));
}

void ResetDefaultBraveBrowserPrompt(Profile* profile) {
  // Don't reset, but keep this function for now as more work is
  // planned in https://github.com/brave/brave-browser/issues/14469
}

void RegisterDefaultBraveBrowserPromptPrefs(PrefRegistrySimple* registry) {
  registry->RegisterIntegerPref(kDefaultBrowserLaunchingCount, 0);
}
