/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_SPEEDREADER_SPEEDREADER_TAB_HELPER_H_
#define BRAVE_BROWSER_SPEEDREADER_SPEEDREADER_TAB_HELPER_H_

#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace content {
class WebContents;
}

namespace speedreader {

// Updates the speedreader toolbar button state when a tab visibility changes.
class SpeedreaderTabHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<SpeedreaderTabHelper> {
 public:
  ~SpeedreaderTabHelper() override;

  SpeedreaderTabHelper(const SpeedreaderTabHelper&) = delete;
  SpeedreaderTabHelper& operator=(SpeedreaderTabHelper&) = delete;

  void SetActive();

 private:
  friend class content::WebContentsUserData<SpeedreaderTabHelper>;
  explicit SpeedreaderTabHelper(content::WebContents* web_contents);

  // content::WebContentsObserver
  void OnVisibilityChanged(content::Visibility visibility) override;

  void UpdateButton();

  bool active_ = false;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace speedreader

#endif  // BRAVE_BROWSER_SPEEDREADER_SPEEDREADER_TAB_HELPER_H_
