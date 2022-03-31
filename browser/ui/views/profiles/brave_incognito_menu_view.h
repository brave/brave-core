/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PROFILES_BRAVE_INCOGNITO_MENU_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_PROFILES_BRAVE_INCOGNITO_MENU_VIEW_H_

#include "chrome/browser/ui/views/profiles/incognito_menu_view.h"

class BraveIncognitoMenuView : public IncognitoMenuView {
 public:
  using IncognitoMenuView::IncognitoMenuView;

  BraveIncognitoMenuView(const BraveIncognitoMenuView&) = delete;
  BraveIncognitoMenuView& operator=(const BraveIncognitoMenuView&) = delete;
  ~BraveIncognitoMenuView() override = default;

  // ProfileMenuViewBase:
  void BuildMenu() override;
  void AddedToWidget() override;

 private:
  friend class IncognitoMenuView;

  // views::BubbleDialogDelegateView:
  std::u16string GetAccessibleWindowTitle() const override;

  // Button actions.
  void OnExitButtonClicked() override;

  void AddTorButton();
  void OnTorProfileButtonClicked();
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PROFILES_BRAVE_INCOGNITO_MENU_VIEW_H_
