/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_VIEWS_PAGE_INFO_BRAVE_SHIELDS_UI_CONTENTS_CACHE_H_
#define BRAVE_BROWSER_UI_VIEWS_PAGE_INFO_BRAVE_SHIELDS_UI_CONTENTS_CACHE_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "base/timer/timer.h"

class WebUIContentsWrapper;

// Manages caching of Brave Shields WebUI contents to improve performance when
// repeatedly opening the Shields panel within the Page Info bubble.
class BraveShieldsUIContentsCache {
 public:
  BraveShieldsUIContentsCache();

  BraveShieldsUIContentsCache(const BraveShieldsUIContentsCache&) = delete;
  BraveShieldsUIContentsCache& operator=(const BraveShieldsUIContentsCache&) =
      delete;

  ~BraveShieldsUIContentsCache();

  // Retrieves and releases ownership of the cached Shields WebUI contents.
  // Returns nullptr if no cached contents are available.
  std::unique_ptr<WebUIContentsWrapper> GetCachedShieldsUIContents();

  // Stores the Shields WebUI contents in the cache and starts the expiry timer.
  // If there is a currently cached WebContents, it will be released.
  void CacheShieldsUIContents(
      std::unique_ptr<WebUIContentsWrapper> contents_wrapper);

  // Clears the cached Shields WebUI contents.
  void ResetCachedShieldsUIContents();

 private:
  std::unique_ptr<WebUIContentsWrapper> contents_wrapper_;
  std::unique_ptr<base::RetainingOneShotTimer> cache_timer_;
};

#endif  // BRAVE_BROWSER_UI_VIEWS_PAGE_INFO_BRAVE_SHIELDS_UI_CONTENTS_CACHE_H_
