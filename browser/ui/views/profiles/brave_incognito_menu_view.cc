/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/profiles/brave_incognito_menu_view.h"

#include <memory>
#include <utility>

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/tor/buildflags.h"
#include "brave/browser/tor/tor_profile.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/hover_button.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/controls/button/button.h"

#if BUILDFLAG(ENABLE_TOR)
#include "brave/browser/extensions/brave_tor_client_updater.h"

namespace {
constexpr int kIconSize = 16;
}  // namespace
#endif

void BraveIncognitoMenuView::ButtonPressed(views::Button* sender,
                                           const ui::Event& event) {
  if (sender == tor_profile_button_) {
    profiles::SwitchToTorProfile(ProfileManager::CreateCallback());
  } else {
    IncognitoMenuView::ButtonPressed(sender, event);
  }
}

void BraveIncognitoMenuView::AddTorButton() {
#if BUILDFLAG(ENABLE_TOR)
  if (!tor::IsTorProfile(browser()->profile()) &&
      !g_brave_browser_process->tor_client_updater()
           ->GetExecutablePath()
           .empty()) {
    tor_profile_button_ = CreateAndAddButton(
        gfx::CreateVectorIcon(kLaunchIcon, kIconSize, gfx::kChromeIconGrey),
        l10n_util::GetStringUTF16(IDS_PROFILES_OPEN_TOR_PROFILE_BUTTON));
  }
#endif
}

void BraveIncognitoMenuView::Reset() {
  IncognitoMenuView::Reset();
  tor_profile_button_ = nullptr;
}
