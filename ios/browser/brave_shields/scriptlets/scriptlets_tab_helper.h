// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_BRAVE_SHIELDS_SCRIPTLETS_SCRIPTLETS_TAB_HELPER_H_
#define BRAVE_IOS_BROWSER_BRAVE_SHIELDS_SCRIPTLETS_SCRIPTLETS_TAB_HELPER_H_

#include <string>
#include <vector>

#include "base/functional/callback_forward.h"
#include "ios/web/public/web_state_user_data.h"

class GURL;
@protocol ScriptletsTabHelperBridge;

namespace web {
class WebState;
}

class ScriptletsTabHelper : public web::WebStateUserData<ScriptletsTabHelper> {
 public:
  ~ScriptletsTabHelper() override;

  void SetBridge(id<ScriptletsTabHelperBridge> bridge);

  void RequestScriptlets(
      const GURL& frame_url,
      base::OnceCallback<void(std::vector<std::string>)> callback);

 private:
  friend class web::WebStateUserData<ScriptletsTabHelper>;

  explicit ScriptletsTabHelper(web::WebState* web_state);

  __weak id<ScriptletsTabHelperBridge> bridge_;
};

#endif  // BRAVE_IOS_BROWSER_BRAVE_SHIELDS_SCRIPTLETS_SCRIPTLETS_TAB_HELPER_H_
