/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_REQUEST_OTR_REQUEST_OTR_TAB_HELPER_H_
#define BRAVE_BROWSER_REQUEST_OTR_REQUEST_OTR_TAB_HELPER_H_

#include "chrome/browser/ui/tabs/contents_observing_tab_feature.h"

namespace tabs {
class TabInterface;
}  // namespace tabs

// Shows an infobar while the user browses a site off the record at its
// request, offering to reload it normally.
class RequestOTRTabHelper : public tabs::ContentsObservingTabFeature {
 public:
  explicit RequestOTRTabHelper(tabs::TabInterface& tab);
  ~RequestOTRTabHelper() override;

  RequestOTRTabHelper(const RequestOTRTabHelper&) = delete;
  RequestOTRTabHelper& operator=(const RequestOTRTabHelper&) = delete;

 private:
  // content::WebContentsObserver:
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;
};

#endif  // BRAVE_BROWSER_REQUEST_OTR_REQUEST_OTR_TAB_HELPER_H_
