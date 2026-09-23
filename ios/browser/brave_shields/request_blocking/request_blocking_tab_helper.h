// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_BRAVE_SHIELDS_REQUEST_BLOCKING_REQUEST_BLOCKING_TAB_HELPER_H_
#define BRAVE_IOS_BROWSER_BRAVE_SHIELDS_REQUEST_BLOCKING_REQUEST_BLOCKING_TAB_HELPER_H_

#include <string>

#include "base/functional/callback_forward.h"
#include "ios/web/public/web_state_user_data.h"

class GURL;
@protocol RequestBlockingTabHelperBridge;

namespace web {
class WebState;
}

class RequestBlockingTabHelper
    : public web::WebStateUserData<RequestBlockingTabHelper> {
 public:
  ~RequestBlockingTabHelper() override;

  void SetBridge(id<RequestBlockingTabHelperBridge> bridge);

  void ShouldBlock(const GURL& request_url,
                   const GURL& source_url,
                   const std::string& resource_type,
                   base::OnceCallback<void(bool)> callback);

 private:
  friend class web::WebStateUserData<RequestBlockingTabHelper>;

  explicit RequestBlockingTabHelper(web::WebState* web_state);

  __weak id<RequestBlockingTabHelperBridge> bridge_;
};

#endif  // BRAVE_IOS_BROWSER_BRAVE_SHIELDS_REQUEST_BLOCKING_REQUEST_BLOCKING_TAB_HELPER_H_
