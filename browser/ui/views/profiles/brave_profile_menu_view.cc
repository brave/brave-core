/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/profiles/brave_profile_menu_view.h"

#include <memory>
#include <utility>

#include "brave/browser/profiles/profile_util.h"
#include "chrome/app/vector_icons/vector_icons.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_window.h"
#include "chrome/browser/ui/browser_finder.h"
#include "chrome/browser/ui/signin/profile_colors_util.h"
#include "chrome/grit/generated_resources.h"
#include "components/vector_icons/vector_icons.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/models/image_model.h"

void BraveProfileMenuView::BuildIdentity() {
  ProfileMenuView::BuildIdentity();
  Profile* profile = browser()->profile();
  ProfileAttributesEntry* profile_attributes =
      g_browser_process->profile_manager()
          ->GetProfileAttributesStorage()
          .GetProfileAttributesWithPath(profile->GetPath());
  // Reset IdentityInfo to get rid of the subtitle string
  // IDS_PROFILES_LOCAL_PROFILE_STATE("Not signed in").
  SetProfileIdentityInfo(
      /*profile_name=*/base::string16(),
      profile_attributes->GetProfileThemeColors().profile_highlight_color,
      /*edit_button=*/base::nullopt,
      ui::ImageModel::FromImage(profile_attributes->GetAvatarIcon()),
      /*title=*/base::string16());
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
    window_count += chrome::GetBrowserCount(profile->GetPrimaryOTRProfile());
  if (window_count > 1) {
    AddFeatureButton(
        l10n_util::GetPluralStringFUTF16(IDS_PROFILES_CLOSE_X_WINDOWS_BUTTON,
                                         window_count),
        base::BindRepeating(&ProfileMenuView::OnExitProfileButtonClicked,
                            base::Unretained(this)),
        vector_icons::kCloseIcon);
  }
}

gfx::ImageSkia BraveProfileMenuView::GetSyncIcon() const {
  // We don't need sync overlay.
  return gfx::ImageSkia();
}

void BraveProfileMenuView::OnExitProfileButtonClicked() {
  if (browser()->profile()->IsGuestSession()) {
    RecordClick(ActionableItem::kExitProfileButton);
    profiles::CloseGuestProfileWindows();
  } else {
    ProfileMenuView::OnExitProfileButtonClicked();
  }
}
