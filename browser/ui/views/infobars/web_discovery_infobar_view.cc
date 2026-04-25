/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/infobars/web_discovery_infobar_view.h"

#include <utility>

#include "brave/browser/ui/views/infobars/web_discovery_infobar_content_view.h"
#include "brave/browser/web_discovery/web_discovery_infobar_delegate.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/views/widget/widget.h"

// web_discovery_tab_helper.cc has decls.
std::unique_ptr<infobars::InfoBar> CreateWebDiscoveryInfoBar(
    std::unique_ptr<WebDiscoveryInfoBarDelegate> delegate) {
  return std::make_unique<WebDiscoveryInfoBarView>(std::move(delegate));
}

WebDiscoveryInfoBarView::WebDiscoveryInfoBarView(
    std::unique_ptr<WebDiscoveryInfoBarDelegate> delegate)
    : InfoBarView(std::move(delegate)) {
  content_view_ = AddChildView(
      std::make_unique<WebDiscoveryInfoBarContentView>(GetDelegate()));
}

WebDiscoveryInfoBarView::~WebDiscoveryInfoBarView() = default;

WebDiscoveryInfoBarDelegate* WebDiscoveryInfoBarView::GetDelegate() {
  return static_cast<WebDiscoveryInfoBarDelegate*>(delegate());
}

void WebDiscoveryInfoBarView::Layout(PassKey) {
  // Don't need to layout base class's elements as our |content_view_| covers
  // all area except bottom separator.
  content_view_->SetBounds(0, OffsetY(content_view_), width(),
                           content_view_->height());
}

void WebDiscoveryInfoBarView::ChildPreferredSizeChanged(views::View* child) {
  if (child == content_view_) {
    child->SizeToPreferredSize();
    SetTargetHeight(child->GetPreferredSize().height());
  }
}

BEGIN_METADATA(WebDiscoveryInfoBarView)
END_METADATA
