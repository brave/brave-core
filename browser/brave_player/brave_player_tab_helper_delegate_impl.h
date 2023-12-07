/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_PLAYER_BRAVE_PLAYER_TAB_HELPER_DELEGATE_IMPL_H_
#define BRAVE_BROWSER_BRAVE_PLAYER_BRAVE_PLAYER_TAB_HELPER_DELEGATE_IMPL_H_

#include "brave/components/brave_player/content/brave_player_tab_helper.h"

class BravePlayerTabHelperDelegateImpl
    : public brave_player::BravePlayerTabHelper::Delegate {
 public:
  BravePlayerTabHelperDelegateImpl();
  ~BravePlayerTabHelperDelegateImpl() override;

  // brave_player::BravePlayerServiceDelegate:
  void ShowAdBlockAdjustmentSuggestion(content::WebContents* contents) override;
};

#endif  // BRAVE_BROWSER_BRAVE_PLAYER_BRAVE_PLAYER_TAB_HELPER_DELEGATE_IMPL_H_
