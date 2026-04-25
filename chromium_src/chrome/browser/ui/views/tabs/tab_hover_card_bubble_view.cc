/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/views/tabs/tab_hover_card_bubble_view.h"

#include <string>

#include "base/strings/strcat.h"
#include "brave/browser/ui/brave_scheme_utils.h"
#include "brave/browser/ui/tabs/brave_tab_prefs.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/tabs/tab.h"
#include "chrome/browser/ui/views/tabs/tab_hover_card_controller.h"
#include "chrome/browser/ui/views/tabs/tab_slot_controller.h"
#include "content/public/common/url_constants.h"
#include "ui/views/controls/label.h"

#define TabHoverCardBubbleView TabHoverCardBubbleView_ChromiumImpl
#include <chrome/browser/ui/views/tabs/tab_hover_card_bubble_view.cc>
#undef TabHoverCardBubbleView

void TabHoverCardBubbleView_ChromiumImpl::BraveUpdateCardContent(
    const HoverCardAnchorTarget* anchor_target) {
  TabHoverCardBubbleView_ChromiumImpl::UpdateCardContent(anchor_target);
  // Replace chrome:// with brave://. Since this is purely in the UI we can
  // just do a sub-string replacement instead of parsing into GURL.
  auto domain = std::u16string(domain_label_->GetText());
  if (brave_utils::ReplaceChromeToBraveScheme(domain)) {
    domain_label_->SetData({domain, /*is_filename*/ false});
  }
}

void TabHoverCardBubbleView::UpdateCardContent(
    const HoverCardAnchorTarget* anchor_target) {
  BraveUpdateCardContent(anchor_target);
}

void TabHoverCardBubbleView::SetTargetTabImage(gfx::ImageSkia preview_image) {
  if (!has_thumbnail_view()) {
    return;
  }
  TabHoverCardBubbleView_ChromiumImpl::SetTargetTabImage(preview_image);
}

void TabHoverCardBubbleView::SetPlaceholderImage() {
  if (!has_thumbnail_view()) {
    return;
  }
  TabHoverCardBubbleView_ChromiumImpl::SetPlaceholderImage();
}
