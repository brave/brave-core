// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/views/tabs/brave_tab_hover_card_controller.h"

#include <memory>

#include "base/functional/bind.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/tabs/tab.h"
#include "chrome/browser/ui/views/tabs/tab_hover_card_bubble_view.h"
#include "chrome/browser/ui/views/tabs/tab_hover_card_controller.h"
#include "chrome/browser/ui/views/tabs/tab_hover_card_thumbnail_observer.h"
#include "chrome/browser/ui/views/tabs/tab_strip.h"

BraveTabHoverCardController::~BraveTabHoverCardController() = default;

void BraveTabHoverCardController::CreateHoverCard(
    HoverCardAnchorTarget* anchor_target) {
  hover_card_image_previews_enabled_ =
      AreHoverCardImagesEnabled() ||
      brave_tabs::AreCardPreviewsEnabled(
          browser_window_interface_->GetProfile()->GetPrefs());

  TabHoverCardController::CreateHoverCard(anchor_target);
}

void BraveTabHoverCardController::OnHovercardImagesEnabledChanged() {
  hover_card_image_previews_enabled_ =
      AreHoverCardImagesEnabled() ||
      brave_tabs::AreCardPreviewsEnabled(
          browser_window_interface_->GetProfile()->GetPrefs());
  if (!hover_card_image_previews_enabled_) {
    thumbnail_subscription_ = base::CallbackListSubscription();
    thumbnail_observer_.reset();
  }
}
