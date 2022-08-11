/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_NTP_BACKGROUND_NTP_TAB_HELPER_H_
#define BRAVE_BROWSER_NTP_BACKGROUND_NTP_TAB_HELPER_H_

#include "base/memory/raw_ptr.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace ntp_background_images {

class ViewCounterService;

class NTPTabHelper : public content::WebContentsObserver,
                     public content::WebContentsUserData<NTPTabHelper> {
 public:
  explicit NTPTabHelper(content::WebContents* web_contents);
  ~NTPTabHelper() override;

  NTPTabHelper(const NTPTabHelper&) = delete;
  NTPTabHelper& operator=(const NTPTabHelper&) = delete;

 private:
  friend class content::WebContentsUserData<NTPTabHelper>;

  // content::WebContentsObserver:
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

  raw_ptr<ViewCounterService> view_counter_service_;

  WEB_CONTENTS_USER_DATA_KEY_DECL();
};

}  // namespace ntp_background_images

#endif  //  BRAVE_BROWSER_NTP_BACKGROUND_NTP_TAB_HELPER_H_
