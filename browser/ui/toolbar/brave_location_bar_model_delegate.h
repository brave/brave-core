/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TOOLBAR_BRAVE_LOCATION_BAR_MODEL_DELEGATE_H_
#define BRAVE_BROWSER_UI_TOOLBAR_BRAVE_LOCATION_BAR_MODEL_DELEGATE_H_

#include "base/compiler_specific.h"
#include "chrome/browser/ui/browser_location_bar_model_delegate.h"

class Browser;

class BraveLocationBarModelDelegate : public BrowserLocationBarModelDelegate {
 public:
  explicit BraveLocationBarModelDelegate(Browser* browser);
  BraveLocationBarModelDelegate(const BraveLocationBarModelDelegate&) = delete;
  BraveLocationBarModelDelegate& operator=(
      const BraveLocationBarModelDelegate&) = delete;
  ~BraveLocationBarModelDelegate() override;
  static void FormattedStringFromURL(const GURL& url,
                                     std::u16string* new_formatted_url);

 private:
  std::u16string FormattedStringWithEquivalentMeaning(
      const GURL& url,
      const std::u16string& formatted_url) const override;
};

#endif  // BRAVE_BROWSER_UI_TOOLBAR_BRAVE_LOCATION_BAR_MODEL_DELEGATE_H_
