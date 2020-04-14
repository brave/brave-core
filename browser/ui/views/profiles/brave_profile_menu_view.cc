/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/profiles/brave_profile_menu_view.h"

#include <memory>
#include <utility>

#include "brave/browser/profiles/profile_util.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/profiles/profile_attributes_entry.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/browser/profiles/profile_window.h"

void BraveProfileMenuView::BuildIdentity() {
  ProfileMenuView::BuildIdentity();
  Profile* profile = browser()->profile();
  ProfileAttributesEntry* profile_attributes;
  g_browser_process->profile_manager()
      ->GetProfileAttributesStorage()
      .GetProfileAttributesWithPath(profile->GetPath(), &profile_attributes);
  // Reset IdentityInfo to get rid of the subtitle string
  // IDS_PROFILES_LOCAL_PROFILE_STATE("Not signed in").
  SetIdentityInfo(profile_attributes->GetAvatarIcon().AsImageSkia(),
                  /*title=*/base::string16());
}

// We don't want autofill buttons in this menu.
void BraveProfileMenuView::BuildAutofillButtons() {}

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
