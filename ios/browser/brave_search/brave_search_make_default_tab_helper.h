// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_BRAVE_SEARCH_BRAVE_SEARCH_MAKE_DEFAULT_TAB_HELPER_H_
#define BRAVE_IOS_BROWSER_BRAVE_SEARCH_BRAVE_SEARCH_MAKE_DEFAULT_TAB_HELPER_H_

#include "ios/web/public/web_state_user_data.h"

@protocol BraveSearchMakeDefaultTabHelperBridge;

namespace web {
class WebState;
}  // namespace web

class BraveSearchMakeDefaultTabHelper
    : public web::WebStateUserData<BraveSearchMakeDefaultTabHelper> {
 public:
  ~BraveSearchMakeDefaultTabHelper() override;

  void SetBridge(id<BraveSearchMakeDefaultTabHelperBridge> bridge);

  bool GetCanSetDefaultSearchProvider();
  void SetIsDefaultSearchProvider();

 private:
  friend class web::WebStateUserData<BraveSearchMakeDefaultTabHelper>;

  explicit BraveSearchMakeDefaultTabHelper(web::WebState* web_state);

  __weak id<BraveSearchMakeDefaultTabHelperBridge> bridge_;
};

#endif  // BRAVE_IOS_BROWSER_BRAVE_SEARCH_BRAVE_SEARCH_MAKE_DEFAULT_TAB_HELPER_H_
