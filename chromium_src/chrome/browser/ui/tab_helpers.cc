/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/brave_tab_helpers.h"

// Include this to make sure we don't unintentionally rename NetErrorTabHelper
// into NoTabHelper inside WebContentsReceiverSet.
#include "content/public/browser/web_contents_receiver_set.h"

#define BRAVE_TAB_HELPERS \
  brave::AttachTabHelpers(web_contents);

// Dummy class for avoiding some TabHelpers from being added to the WebContents.
class NoTabHelper {
 public:
  template <typename... Args>
  static void CreateForWebContents(content::WebContents* contents,
                                   Args&&... args) {}
};

// For each TabHelper we want to disable we need to do two things:
//  1. Redefine below this point the namespaces of the TabHelpers we want to
//     disable to empty, so that the NoTabHelper class from above can be found.
//  2. Redefine the TabHelper we want to disable to NoTabHelper.
//
// Please keep all the redefinitions for disabling TabHelpers below this point.
#define chrome_browser_net
#define NetErrorTabHelper NoTabHelper

#include "../../../../../chrome/browser/ui/tab_helpers.cc"

#undef NetErrorTabHelper
#undef chrome_browser_net
#undef BRAVE_TAB_HELPERS
