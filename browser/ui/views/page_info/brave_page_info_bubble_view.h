/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PAGE_INFO_BRAVE_PAGE_INFO_BUBBLE_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_PAGE_INFO_BRAVE_PAGE_INFO_BUBBLE_VIEW_H_

#include <utility>

#include "chrome/browser/ui/views/page_info/page_info_bubble_view.h"
#include "ui/base/metadata/metadata_header_macros.h"

// Brave-customized version of Chromium's page info bubble, which displays
// shields, permission, and security information for the current site.
class BravePageInfoBubbleView : public PageInfoBubbleView {
  METADATA_HEADER(BravePageInfoBubbleView, PageInfoBubbleView)

 public:
  BravePageInfoBubbleView(const BravePageInfoBubbleView&) = delete;
  BravePageInfoBubbleView& operator=(const BravePageInfoBubbleView&) = delete;

  ~BravePageInfoBubbleView() override;

  // PageInfoBubbleView:
  void OpenMainPage(base::OnceClosure initialized_callback) override;
  void AnnouncePageOpened(std::u16string announcement) override;

 private:
  friend class PageInfoBubbleView;

  template <typename... Args>
  explicit BravePageInfoBubbleView(Args&&... args)
      : PageInfoBubbleView(std::forward<Args>(args)...) {
    InitializeView();
  }

  void InitializeView();

  // Applies Brave-specific customizations to the Chromium page info views.
  void CustomizeChromiumViews();
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PAGE_INFO_BRAVE_PAGE_INFO_BUBBLE_VIEW_H_
