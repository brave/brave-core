/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/tabs/tab_hover_card_bubble_view.h"
#include "chrome/browser/ui/views/tabs/tab_hover_card_controller.h"
#include "chrome/browser/ui/views/tabs/tab_slot_controller.h"

// In addition to the feature flag, we also enable hover card previews when
// the tab hover mode is CARD_WITH_PREVIEW.
#define AreHoverCardImagesEnabled()       \
  AreHoverCardImagesEnabled() ||          \
      brave_tabs::AreCardPreviewsEnabled( \
          tab->controller()->GetBrowser()->profile()->GetPrefs())

#define TabHoverCardBubbleView TabHoverCardBubbleView_ChromiumImpl
#include "src/chrome/browser/ui/views/tabs/tab_hover_card_bubble_view.cc"
#undef TabHoverCardBubbleView
#undef AreHoverCardImagesEnabled

void TabHoverCardBubbleView::SetTargetTabImage(gfx::ImageSkia preview_image) {
  if (!has_thumbnail_view())
    return;
  TabHoverCardBubbleView_ChromiumImpl::SetTargetTabImage(preview_image);
}

void TabHoverCardBubbleView::SetPlaceholderImage() {
  if (!has_thumbnail_view())
    return;
  TabHoverCardBubbleView_ChromiumImpl::SetPlaceholderImage();
}
