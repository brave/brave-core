/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_INFOBARS_WEB_DISCOVERY_INFOBAR_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_INFOBARS_WEB_DISCOVERY_INFOBAR_VIEW_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "chrome/browser/ui/views/infobars/infobar_view.h"
#include "ui/base/metadata/metadata_header_macros.h"

class WebDiscoveryInfoBarDelegate;

class WebDiscoveryInfoBarView : public InfoBarView {
  METADATA_HEADER(WebDiscoveryInfoBarView, InfoBarView)
 public:

  explicit WebDiscoveryInfoBarView(
      std::unique_ptr<WebDiscoveryInfoBarDelegate> delegate);
  ~WebDiscoveryInfoBarView() override;

  WebDiscoveryInfoBarView(const WebDiscoveryInfoBarView&) = delete;
  WebDiscoveryInfoBarView& operator=(const WebDiscoveryInfoBarView&) = delete;

 private:
  // InfoBarView overrides:
  void Layout(PassKey) override;
  void ChildPreferredSizeChanged(views::View* child) override;

  WebDiscoveryInfoBarDelegate* GetDelegate();

  raw_ptr<views::View> content_view_ = nullptr;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_INFOBARS_WEB_DISCOVERY_INFOBAR_VIEW_H_
