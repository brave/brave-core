/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_BRAVE_TAB_STRIP_MODEL_DELEGATE_H_
#define BRAVE_BROWSER_UI_BRAVE_TAB_STRIP_MODEL_DELEGATE_H_

#include <memory>
#include <vector>

#include "chrome/browser/ui/browser_tab_strip_model_delegate.h"

// In order to make it easy to replace BrowserTabStripModelDelegate with our
// BraveTabStripModelDelegate, wrap this class with chrome namespace.
namespace chrome {

class BraveTabStripModelDelegate : public BrowserTabStripModelDelegate {
 public:
  using BrowserTabStripModelDelegate::BrowserTabStripModelDelegate;
  BraveTabStripModelDelegate(const BraveTabStripModelDelegate&) = delete;
  BraveTabStripModelDelegate& operator=(const BraveTabStripModelDelegate&) =
      delete;
  ~BraveTabStripModelDelegate() override = default;

  // BrowserTabStripModelDelegate:
  bool CanMoveTabsToWindow(const std::vector<int>& indices) override;
  void CacheWebContents(const std::vector<std::unique_ptr<DetachedWebContents>>&
                            web_contents) override;
};

}  // namespace chrome

#endif  // BRAVE_BROWSER_UI_BRAVE_TAB_STRIP_MODEL_DELEGATE_H_
