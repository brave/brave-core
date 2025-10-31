/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/page_info/brave_page_info_bubble_view.h"

#include "brave/browser/ui/page_info/features.h"
#include "chrome/browser/ui/views/page_info/page_info_view_factory.h"
#include "ui/base/metadata/metadata_impl_macros.h"

BravePageInfoBubbleView::~BravePageInfoBubbleView() = default;

void BravePageInfoBubbleView::OpenMainPage(
    base::OnceClosure initialized_callback) {
  PageInfoBubbleView::OpenMainPage(std::move(initialized_callback));
  CustomizeChromiumViews();
}

void BravePageInfoBubbleView::AnnouncePageOpened(std::u16string announcement) {
  PageInfoBubbleView::AnnouncePageOpened(announcement);
  CustomizeChromiumViews();
}

void BravePageInfoBubbleView::InitializeView() {
  CustomizeChromiumViews();
}

void BravePageInfoBubbleView::CustomizeChromiumViews() {
  if (!page_info::features::IsShowBraveShieldsInPageInfoEnabled()) {
    return;
  }

  // Hide the close button in the page header.
  auto* close_button =
      GetViewByID(PageInfoViewFactory::VIEW_ID_PAGE_INFO_CLOSE_BUTTON);
  if (close_button) {
    close_button->SetVisible(false);
  }
}

BEGIN_METADATA(BravePageInfoBubbleView)
END_METADATA
