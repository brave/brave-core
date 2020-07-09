/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_FARBLING_FARBLING_TAB_HELPER_H_
#define BRAVE_BROWSER_FARBLING_FARBLING_TAB_HELPER_H_

#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class NavigationHandle;
class WebContents;
}  // namespace content

namespace brave {

class FarblingTabHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<FarblingTabHelper> {
 public:
  ~FarblingTabHelper() override;

  FarblingTabHelper(const FarblingTabHelper&) = delete;
  FarblingTabHelper& operator=(FarblingTabHelper&) = delete;

 private:
  friend class content::WebContentsUserData<FarblingTabHelper>;
  explicit FarblingTabHelper(content::WebContents* web_contents);

  void UpdateUserAgent(content::NavigationHandle* navigation_handle);

  // content::WebContentsObserver
  void DidStartNavigation(
      content::NavigationHandle* navigation_handle) override;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace brave

#endif  // BRAVE_BROWSER_FARBLING_FARBLING_TAB_HELPER_H_
