// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_BRAVE_TALK_BRAVE_TALK_TAB_HELPER_H_
#define BRAVE_IOS_BROWSER_BRAVE_TALK_BRAVE_TALK_TAB_HELPER_H_

#include <string>

#include "base/memory/raw_ptr.h"
#include "ios/web/public/web_state_user_data.h"

@protocol BraveTalkTabHelperBridge;

namespace web {
class WebState;
}  // namespace web

class BraveTalkTabHelper : public web::WebStateUserData<BraveTalkTabHelper> {
 public:
  ~BraveTalkTabHelper() override;

  void SetBridge(id<BraveTalkTabHelperBridge> bridge);

  // Forwards a launch request to the registered delegate, if any.
  void LaunchBraveTalk(const std::string_view room, const std::string_view jwt);

 private:
  friend class web::WebStateUserData<BraveTalkTabHelper>;

  explicit BraveTalkTabHelper(web::WebState* web_state);

  raw_ptr<web::WebState> web_state_ = nullptr;  // not owned
  __weak id<BraveTalkTabHelperBridge> bridge_;
};

#endif  // BRAVE_IOS_BROWSER_BRAVE_TALK_BRAVE_TALK_TAB_HELPER_H_
