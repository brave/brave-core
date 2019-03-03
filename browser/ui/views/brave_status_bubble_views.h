/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_BRAVE_STATUS_BUBBLE_VIEWS_H_
#define BRAVE_BROWSER_UI_VIEWS_BRAVE_STATUS_BUBBLE_VIEWS_H_

#include "chrome/browser/ui/views/status_bubble_views.h"

class BraveStatusBubbleViews : public StatusBubbleViews {
 public:
  using StatusBubbleViews::StatusBubbleViews;
  ~BraveStatusBubbleViews() override = default;

  // StatusBubbleViews overrides:
  void SetURL(const GURL& url) override;

 private:
  DISALLOW_COPY_AND_ASSIGN(BraveStatusBubbleViews);
};

#endif  // BRAVE_BROWSER_UI_VIEWS_BRAVE_STATUS_BUBBLE_VIEWS_H_
