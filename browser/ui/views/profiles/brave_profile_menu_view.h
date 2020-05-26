/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PROFILES_BRAVE_PROFILE_MENU_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_PROFILES_BRAVE_PROFILE_MENU_VIEW_H_

#include "chrome/browser/ui/views/profiles/profile_menu_view.h"

class BraveProfileMenuView : public ProfileMenuView {
 private:
  friend class ProfileMenuView;

  using ProfileMenuView::ProfileMenuView;
  ~BraveProfileMenuView() override = default;

  // Button/link actions.
  void OnExitProfileButtonClicked() override;

  // Helper methods for building the menu.
  void BuildIdentity() override;
  void BuildAutofillButtons() override;
  gfx::ImageSkia GetSyncIcon() const override;

  DISALLOW_COPY_AND_ASSIGN(BraveProfileMenuView);
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PROFILES_BRAVE_PROFILE_MENU_VIEW_H_
