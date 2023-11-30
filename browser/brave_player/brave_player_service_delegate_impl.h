/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_BRAVE_PLAYER_BRAVE_PLAYER_SERVICE_DELEGATE_IMPL_H_
#define BRAVE_BROWSER_BRAVE_PLAYER_BRAVE_PLAYER_SERVICE_DELEGATE_IMPL_H_

#include "brave/components/brave_player/core/browser/brave_player_service.h"

class BravePlayerServiceDelegateImpl
    : public brave_player::BravePlayerService::Delegate {
 public:
  BravePlayerServiceDelegateImpl();
  ~BravePlayerServiceDelegateImpl() override;

  // brave_player::BravePlayerServiceDelegate:
  void ShowAdBlockAdjustmentSuggestion(content::WebContents* contents) override;
};

#endif  // BRAVE_BROWSER_BRAVE_PLAYER_BRAVE_PLAYER_SERVICE_DELEGATE_IMPL_H_
