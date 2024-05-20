/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TABS_SHARED_PINNED_TAB_DUMMY_VIEW_H_
#define BRAVE_BROWSER_UI_TABS_SHARED_PINNED_TAB_DUMMY_VIEW_H_

#include <memory>

namespace content {
class WebContents;
}  // namespace content

// This class is UI toolkit independent interface to install a overlay view
// on dummy contents of shared pinned tabs. Unlike other interstitial pages,
// this view will be implemented with native views toolkit. We have some
// benefits like
// * Don't need N number of renderer process to show the view
// * Don't need to plumb tabs data to web ui.
class SharedPinnedTabDummyView {
 public:
  SharedPinnedTabDummyView() = default;
  virtual ~SharedPinnedTabDummyView() = default;

  static std::unique_ptr<SharedPinnedTabDummyView> Create(
      content::WebContents* shared_contents,
      content::WebContents* dummy_contents);

  // Install the view on given web contents. Note that the view will be
  // uninstalled automatically by the web view.
  virtual void Install() = 0;
};

#endif  // BRAVE_BROWSER_UI_TABS_SHARED_PINNED_TAB_DUMMY_VIEW_H_
