/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/profiles/brave_profile_menu_view.h"

#include <memory>
#include <optional>
#include <utility>

#include "brave/browser/profiles/profile_util.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/profiles/profile_colors_util.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/grit/generated_resources.h"
#include "components/vector_icons/vector_icons.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/image_model.h"

// This method is overriden because otherwise we would have to call
// `SetProfileIdentityInfo` a second time, and this leaves
// `profile_background_container_`, and `heading_label_` dangling in
// `ProfileMenuViewBase` for a while in between
// `identity_info_container_->RemoveAllChildViews()` being called, and the new
// pointers being assigned to those members.
void BraveProfileMenuView::SetProfileIdentityInfo(
    const std::u16string& profile_name,
    SkColor profile_background_color,
    std::optional<EditButtonParams> edit_button_params,
    const ui::ImageModel& image_model,
    const ui::ImageModel& management_badge,
    const std::u16string& title,
    const std::u16string& subtitle,
    const std::u16string& management_label,
    const gfx::VectorIcon* header_art_icon) {
  // For non-guest sessions, we want to eliminate the subtitle
  // IDS_PROFILES_LOCAL_PROFILE_STATE("Not signed in"). In order to do that, we
  // must fetch the desired title here so that we can pass it in along with the
  // given subtitle below.
  Profile* profile = browser()->profile();
  std::u16string desired_title = title;
  if (!profile->IsGuestSession()) {
    ProfileAttributesEntry* profile_attributes =
        g_browser_process->profile_manager()
            ->GetProfileAttributesStorage()
            .GetProfileAttributesWithPath(profile->GetPath());
    desired_title = profile_attributes->GetName();
  }

  // We never show the profile name (displayed above the user avatar) nor the
  // edit buttons, so pass in default values for those parameters.
  ProfileMenuView::SetProfileIdentityInfo(
      /*profile_name=*/std::u16string(), profile_background_color,
      /*edit_button_params=*/std::nullopt, image_model, management_badge,
      desired_title, subtitle, management_label, header_art_icon);
}

// We don't want autofill buttons in this menu.
void BraveProfileMenuView::BuildAutofillButtons() {}

// We don't want to show any Chromium sync info.
void BraveProfileMenuView::BuildSyncInfo() {}

// We don't want feature buttons to manage google account
void BraveProfileMenuView::BuildFeatureButtons() {
  Profile* profile = browser()->profile();
  int window_count = chrome::GetBrowserCount(profile);
  if (!profile->IsOffTheRecord() && profile->HasPrimaryOTRProfile())
    window_count += chrome::GetBrowserCount(
        profile->GetPrimaryOTRProfile(/*create_if_needed=*/true));
  if (profile->IsGuestSession()) {
    AddFeatureButton(
        l10n_util::GetPluralStringFUTF16(IDS_GUEST_PROFILE_MENU_CLOSE_BUTTON,
                                         window_count),
        base::BindRepeating(&ProfileMenuView::OnExitProfileButtonClicked,
                            base::Unretained(this)),
        vector_icons::kCloseIcon);
  } else {
    if (window_count > 1) {
      AddFeatureButton(
          l10n_util::GetPluralStringFUTF16(IDS_PROFILES_CLOSE_X_WINDOWS_BUTTON,
                                           window_count),
          base::BindRepeating(&ProfileMenuView::OnExitProfileButtonClicked,
                              base::Unretained(this)),
          vector_icons::kCloseIcon);
    }
  }
}

gfx::ImageSkia BraveProfileMenuView::GetSyncIcon() const {
  // We don't need sync overlay.
  return gfx::ImageSkia();
}
