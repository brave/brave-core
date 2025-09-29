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
#include "ui/views/bubble/bubble_border.h"

BraveTabHoverCardController::~BraveTabHoverCardController() = default;

void BraveTabHoverCardController::SetIsVerticalTabs(bool is_vertical_tabs) {
  if (std::exchange(is_vertical_tabs_, is_vertical_tabs) == is_vertical_tabs) {
    return;
  }

  UpdateHoverCardArrow();
}

void BraveTabHoverCardController::UpdateHoverCardArrow() {
  if (hover_card_) {
    hover_card_->SetArrow(is_vertical_tabs_ ? views::BubbleBorder::LEFT_TOP
                                            : views::BubbleBorder::TOP_CENTER);
  }
}

void BraveTabHoverCardController::CreateHoverCard(Tab* tab) {
  hover_card_image_previews_enabled_ =
      AreHoverCardImagesEnabled() ||
      brave_tabs::AreCardPreviewsEnabled(
          tab->controller()->GetBrowser()->profile()->GetPrefs());

  TabHoverCardController::CreateHoverCard(tab);

  UpdateHoverCardArrow();
}

void BraveTabHoverCardController::OnHovercardImagesEnabledChanged() {
  hover_card_image_previews_enabled_ =
      AreHoverCardImagesEnabled() ||
      brave_tabs::AreCardPreviewsEnabled(
          tab_strip_->GetBrowser()->profile()->GetPrefs());
  if (!hover_card_image_previews_enabled_) {
    thumbnail_subscription_ = base::CallbackListSubscription();
    thumbnail_observer_.reset();
  }
}
