/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_REQUEST_OTR_REQUEST_OTR_TAB_HELPER_H_
#define BRAVE_BROWSER_REQUEST_OTR_REQUEST_OTR_TAB_HELPER_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "content/public/browser/web_contents_observer.h"
#include "content/public/browser/web_contents_user_data.h"

class PrefService;

class RequestOTRTabHelper
    : public content::WebContentsObserver,
      public content::WebContentsUserData<RequestOTRTabHelper> {
 public:
  explicit RequestOTRTabHelper(content::WebContents* contents);
  ~RequestOTRTabHelper() override;

  RequestOTRTabHelper(const RequestOTRTabHelper&) = delete;
  RequestOTRTabHelper& operator=(const RequestOTRTabHelper&) = delete;

  WEB_CONTENTS_USER_DATA_KEY_DECL();

 private:
  // content::WebContentsObserver overrides:
  void DidFinishNavigation(
      content::NavigationHandle* navigation_handle) override;

  raw_ptr<PrefService> pref_service_ = nullptr;

  base::WeakPtrFactory<RequestOTRTabHelper> weak_factory_;
};

#endif  // BRAVE_BROWSER_REQUEST_OTR_REQUEST_OTR_TAB_HELPER_H_
