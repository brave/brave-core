/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#include <string>

#include "chrome/browser/ui/views/tabs/tab.h"
#include "chrome/browser/ui/views/tabs/tab_hover_card_bubble_view.h"
#include "ui/views/controls/label.h"
#include "brave/common/url_utils.h"

#define TabHoverCardBubbleView TabHoverCardBubbleView_ChromiumImpl
#include "../../../../../../chrome/browser/ui/views/tabs/tab_hover_card_bubble_view.cc"
#undef TabHoverCardBubbleView

void TabHoverCardBubbleView_ChromiumImpl::BraveUpdateCardContent(
    const Tab* tab) {
  TabHoverCardBubbleView_ChromiumImpl::UpdateCardContent(tab);
  const base::string16& domain = domain_label_->GetText();
  // Replace chrome:// with brave://. Since this is purely in the UI we can
  // just do a sub-string replacement instead of parsing into GURL.
  domain_label_->SetText(brave::ReplaceChromeSchemeWithBrave(domain));
}

void TabHoverCardBubbleView::UpdateCardContent(const Tab* tab) {
  BraveUpdateCardContent(tab);
}
