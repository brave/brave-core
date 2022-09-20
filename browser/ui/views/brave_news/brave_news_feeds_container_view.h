// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_NEWS_BRAVE_NEWS_FEEDS_CONTAINER_VIEW_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_NEWS_BRAVE_NEWS_FEEDS_CONTAINER_VIEW_H_

#include "ui/views/view.h"

namespace content {
class WebContents;
}

class BraveNewsFeedsContainerView {
 public:
  explicit BraveNewsFeedsContainerView(content::WebContents* contents);
  BraveNewsFeedsContainerView(const BraveNewsFeedsContainerView&) = delete;
  BraveNewsFeedsContainerView& operator=(const BraveNewsFeedsContainerView&) =
      delete;
  ~BraveNewsFeedsContainerView();
};

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_NEWS_BRAVE_NEWS_FEEDS_CONTAINER_VIEW_H_
