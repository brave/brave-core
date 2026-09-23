/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_TABS_SHARED_PINNED_TAB_DUMMY_VIEW_H_
#define BRAVE_BROWSER_UI_TABS_SHARED_PINNED_TAB_DUMMY_VIEW_H_

#include "base/memory/raw_ptr.h"

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
  // Install the view on given web contents by transferring ownership of dummy
  // view to the web view. The dummy view will be uninstalled automatically when
  // the web view's web contents changes.
  static void CreateAndInstall(content::WebContents* shared_contents,
                               content::WebContents* dummy_contents);

  virtual ~SharedPinnedTabDummyView();

 protected:
  SharedPinnedTabDummyView(content::WebContents* shared_contents,
                           content::WebContents* dummy_contents);

  raw_ptr<content::WebContents> shared_contents_;
  raw_ptr<content::WebContents> dummy_contents_;
};

#endif  // BRAVE_BROWSER_UI_TABS_SHARED_PINNED_TAB_DUMMY_VIEW_H_
