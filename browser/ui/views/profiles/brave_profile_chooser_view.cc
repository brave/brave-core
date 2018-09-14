/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/profiles/brave_profile_chooser_view.h"

#include "brave/browser/brave_browser_process_impl.h"
#include "brave/browser/extensions/brave_tor_client_updater.h"
#include "brave/grit/brave_generated_resources.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/ui/views/hover_button.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/controls/button/label_button.h"
#include "ui/views/layout/grid_layout.h"

namespace {
constexpr int kIconSize = 16;
}  // namespace

BraveProfileChooserView::BraveProfileChooserView(
    views::Button* anchor_button,
    Browser* browser,
    profiles::BubbleViewMode view_mode,
    signin::GAIAServiceType service_type,
    signin_metrics::AccessPoint access_point)
  : ProfileChooserView(anchor_button, browser, view_mode, service_type,
                       access_point) {}

BraveProfileChooserView::~BraveProfileChooserView() {}


void BraveProfileChooserView::ButtonPressed(views::Button* sender,
                                            const ui::Event& event) {
  if (sender == tor_profile_button_) {
    profiles::SwitchToTorProfile(ProfileManager::CreateCallback());
  } else if (sender == users_button_ && browser_->profile()->IsGuestSession()) {
    if (browser_->profile()->IsTorProfile())
      profiles::CloseTorProfileWindows();
    else
      profiles::CloseGuestProfileWindows();
  } else {
    ProfileChooserView::ButtonPressed(sender, event);
  }
}

void BraveProfileChooserView::AddTorButton(views::GridLayout* layout) {
  if (!browser_->profile()->IsTorProfile() &&
      !g_brave_browser_process->tor_client_updater()
        ->GetExecutablePath().empty()) {
    tor_profile_button_ = new HoverButton(this,
      gfx::CreateVectorIcon(kLaunchIcon, kIconSize,
        gfx::kChromeIconGrey),
      l10n_util::GetStringUTF16(IDS_PROFILES_OPEN_TOR_PROFILE_BUTTON));
    layout->StartRow(1.0, 0);
    layout->AddView(tor_profile_button_);
  }
}

void BraveProfileChooserView::ResetView() {
  ProfileChooserView::ResetView();
  tor_profile_button_ = nullptr;
}
