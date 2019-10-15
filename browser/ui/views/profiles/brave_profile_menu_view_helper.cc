/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/profiles/brave_profile_menu_view_helper.h"

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/profiles/profile_util.h"
#include "brave/browser/tor/buildflags.h"
#include "brave/common/pref_names.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "components/prefs/pref_service.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/paint_vector_icon.h"

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/extensions/brave_tor_client_updater.h"
#endif

namespace {
constexpr int kIconSize = 16;
}  // namespace

namespace brave {

bool ShouldShowTorProfileButton(Profile* profile) {
  DCHECK(profile);
#if BUILDFLAG(ENABLE_TOR)
  return !profile->GetPrefs()->GetBoolean(kTorDisabled) &&
      !brave::IsTorProfile(profile) &&
      !g_brave_browser_process->tor_client_updater()->GetExecutablePath()
           .empty();
#else
  return false;
#endif
}

gfx::ImageSkia CreateTorProfileButtonIcon() {
  return gfx::CreateVectorIcon(kLaunchIcon, kIconSize, gfx::kChromeIconGrey);
}

base::string16 CreateTorProfileButtonText() {
  return l10n_util::GetStringUTF16(IDS_PROFILES_OPEN_TOR_PROFILE_BUTTON);
}

}  // namespace brave
