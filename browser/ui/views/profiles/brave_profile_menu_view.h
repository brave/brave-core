/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PROFILES_BRAVE_PROFILE_MENU_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_PROFILES_BRAVE_PROFILE_MENU_VIEW_H_

#include "chrome/browser/ui/views/profiles/profile_menu_view.h"

class BraveProfileMenuView : public ProfileMenuView {
 public:
  BraveProfileMenuView(const BraveProfileMenuView&) = delete;
  BraveProfileMenuView& operator=(const BraveProfileMenuView&) = delete;
  ~BraveProfileMenuView() override = default;

 private:
  friend class ProfileMenuView;
  using ProfileMenuView::ProfileMenuView;

  // Helper methods for building the menu.
  void SetProfileIdentityInfo(
      const ui::ImageModel& image_model,
      const std::u16string& title,
      const std::u16string& subtitle = std::u16string(),
      const gfx::VectorIcon* header_art_icon = nullptr) override;
  void BuildFeatureButtons() override;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PROFILES_BRAVE_PROFILE_MENU_VIEW_H_
