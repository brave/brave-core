/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/browser_command_controller.h"
#include "chrome/browser/ui/browser_content_setting_bubble_model_delegate.h"
#include "brave/browser/ui/brave_browser_content_setting_bubble_model_delegate.h"
#include "brave/browser/ui/brave_browser_command_controller.h"
#include "brave/browser/ui/toolbar/brave_location_bar_model_delegate.h"

class Profile;
namespace brave {
void MaybeDestroyTorProfile(Profile* const profile);
}  // namespace brave

#define BrowserContentSettingBubbleModelDelegate \
  BraveBrowserContentSettingBubbleModelDelegate
#define BrowserCommandController BraveBrowserCommandController
#define BrowserLocationBarModelDelegate BraveLocationBarModelDelegate
#include "../../../../../chrome/browser/ui/browser.cc"  // NOLINT
#undef BrowserLocationBarModelDelegate
#undef BrowserContentSettingBubbleModelDelegate
#undef BrowserCommandController

namespace brave {

void MaybeDestroyTorProfile(Profile* const profile) {
  if (!profile || !profile->IsTorProfile() ||
      BrowserList::IsIncognitoSessionInUse(profile)) {
    return;
  }

#if BUILDFLAG(ENABLE_PRINT_PREVIEW)
  // The Printing Background Manager holds onto preview dialog WebContents
  // whose corresponding print jobs have not yet fully spooled. Make sure
  // these get destroyed before tearing down the incognito profile so that
  // their render frame hosts can exit in time - see crbug.com/579155
  g_browser_process->background_printing_manager()
    ->DeletePreviewContentsForBrowserContext(profile);
#endif
  // A tor profile is no longer needed, this indirectly frees
  // its cache and cookies once it gets destroyed at the appropriate time.
  ProfileDestroyer::DestroyProfileWhenAppropriate(profile);
  return;
}

}  // namespace brave
