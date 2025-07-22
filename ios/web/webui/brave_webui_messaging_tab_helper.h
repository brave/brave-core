// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_WEB_WEBUI_BRAVE_WEBUI_MESSAGING_TAB_HELPER_H_
#define BRAVE_IOS_WEB_WEBUI_BRAVE_WEBUI_MESSAGING_TAB_HELPER_H_

#include <Foundation/Foundation.h>

#include "ios/web/public/web_state.h"
#include "ios/web/public/web_state_observer.h"
#include "ios/web/public/web_state_user_data.h"

@protocol BraveWebUIMessagingTabHelperDelegate;

class BraveWebUIMessagingTabHelper
    : public web::WebStateUserData<BraveWebUIMessagingTabHelper> {
 public:
  BraveWebUIMessagingTabHelper(const BraveWebUIMessagingTabHelper&) = delete;
  BraveWebUIMessagingTabHelper& operator=(const BraveWebUIMessagingTabHelper&) =
      delete;
  ~BraveWebUIMessagingTabHelper() override;

  id<BraveWebUIMessagingTabHelperDelegate> GetBridgingDelegate();
  void SetBridgingDelegate(id<BraveWebUIMessagingTabHelperDelegate> delegate);

 private:
  friend class web::WebStateUserData<BraveWebUIMessagingTabHelper>;

  explicit BraveWebUIMessagingTabHelper(web::WebState* web_state);

  id<BraveWebUIMessagingTabHelperDelegate> bridge_;
  base::WeakPtrFactory<BraveWebUIMessagingTabHelper> weak_ptr_factory_{this};
};

#endif  // BRAVE_IOS_WEB_WEBUI_BRAVE_WEBUI_MESSAGING_TAB_HELPER_H_
