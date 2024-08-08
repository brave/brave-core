/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_SPLIT_VIEW_SPLIT_VIEW_LOCATION_BAR_MODEL_DELEGATE_H_
#define BRAVE_BROWSER_UI_VIEWS_SPLIT_VIEW_SPLIT_VIEW_LOCATION_BAR_MODEL_DELEGATE_H_

#include "chrome/browser/ui/toolbar/chrome_location_bar_model_delegate.h"

namespace content {
class WebContents;
}  // namespace content

class SplitViewLocationBarModelDelegate
    : public ChromeLocationBarModelDelegate {
 public:
  SplitViewLocationBarModelDelegate();
  SplitViewLocationBarModelDelegate(const SplitViewLocationBarModelDelegate&) =
      delete;
  SplitViewLocationBarModelDelegate& operator=(
      const SplitViewLocationBarModelDelegate&) = delete;
  ~SplitViewLocationBarModelDelegate() override;

  void set_web_contents(content::WebContents* web_contents) {
    web_contents_ = web_contents;
  }

  // ChromeLocationBarModelDelegate:
  content::WebContents* GetActiveWebContents() const override;
  bool ShouldDisplayURL() const override;

 private:
  raw_ptr<content::WebContents> web_contents_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_SPLIT_VIEW_SPLIT_VIEW_LOCATION_BAR_MODEL_DELEGATE_H_
