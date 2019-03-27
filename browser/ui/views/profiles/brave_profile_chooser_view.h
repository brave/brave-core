/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PROFILES_BRAVE_PROFILE_CHOOSER_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_PROFILES_BRAVE_PROFILE_CHOOSER_VIEW_H_

#include "chrome/browser/ui/views/profiles/profile_chooser_view.h"

class BraveProfileChooserView : public ProfileChooserView {
 private:
  friend class ProfileChooserView;

  BraveProfileChooserView(views::Button* anchor_button,
                          const gfx::Rect& anchor_rect,
                          gfx::NativeView parent_window,
                          Browser* browser,
                          profiles::BubbleViewMode view_mode,
                          signin::GAIAServiceType service_type,
                          signin_metrics::AccessPoint access_point,
                          bool is_source_keyboard);
  ~BraveProfileChooserView() override;

  // views::ButtonListener:
  void ButtonPressed(views::Button* sender, const ui::Event& event) override;

  void ResetView() override;

  views::View* BraveCreateDiceSyncErrorView(
      const AvatarMenu::Item& avatar_item,
      sync_ui_util::AvatarSyncErrorType error,
      int button_string_id);

  void AddTorButton(views::GridLayout* layout);

  views::LabelButton* tor_profile_button_;

  DISALLOW_COPY_AND_ASSIGN(BraveProfileChooserView);
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PROFILES_BRAVE_PROFILE_CHOOSER_VIEW_H_
