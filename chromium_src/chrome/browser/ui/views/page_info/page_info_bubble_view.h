/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PAGE_INFO_PAGE_INFO_BUBBLE_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PAGE_INFO_PAGE_INFO_BUBBLE_VIEW_H_

class PageInfoBubbleView;

#define HandleMoreInfoRequest                  \
  BraveAddIPFSButtons(PageInfoBubbleView* ui); \
  void HandleMoreInfoRequest

#include "../../../../../../../chrome/browser/ui/views/page_info/page_info_bubble_view.h"
#undef HandleMoreInfoRequest

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_PAGE_INFO_PAGE_INFO_BUBBLE_VIEW_H_
