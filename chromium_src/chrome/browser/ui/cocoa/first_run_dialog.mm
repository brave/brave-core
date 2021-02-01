/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/buildflags.h"
#include "chrome/browser/browser_process.h"
#include "components/prefs/pref_service.h"

#if BUILDFLAG(BRAVE_P3A_ENABLED)
#include "brave/components/p3a/pref_names.h"
#endif

#define ShowFirstRunDialog ShowFirstRunDialog_UnUsed
#include "../../../../../../chrome/browser/ui/cocoa/first_run_dialog.mm"
#undef ShowFirstRunDialog

namespace {

// Copied ShowFirstRunModal from upstream and p3a prefs part added.
void ShowFirstRunModalBrave(Profile* profile) {
  base::scoped_nsobject<FirstRunDialogController> dialog(
      [[FirstRunDialogController alloc] init]);

  [dialog.get() showWindow:nil];

  // If the dialog asked the user to opt-in for stats and crash reporting,
  // record the decision and enable the crash reporter if appropriate.
  bool consent_given = [dialog.get() isStatsReportingEnabled];
  ChangeMetricsReportingState(consent_given);

#if BUILDFLAG(BRAVE_P3A_ENABLED)
  PrefService* local_state = g_browser_process->local_state();
  local_state->SetBoolean(brave::kP3AEnabled, consent_given);
#endif

  // If selected, set as default browser. Skip in automated tests so that an OS
  // dialog confirming the default browser choice isn't left on screen.
  BOOL make_default_browser =
      [dialog.get() isMakeDefaultBrowserEnabled] &&
      !base::CommandLine::ForCurrentProcess()->HasSwitch(switches::kTestType);
  if (make_default_browser) {
    bool success = shell_integration::SetAsDefaultBrowser();
    DCHECK(success);
  }
}
}  // namespace

namespace first_run {

void ShowFirstRunDialog(Profile* profile) {
  ShowFirstRunModalBrave(profile);
}

}  // namespace first_run
