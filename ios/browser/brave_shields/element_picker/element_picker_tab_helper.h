// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_BRAVE_SHIELDS_ELEMENT_PICKER_ELEMENT_PICKER_TAB_HELPER_H_
#define BRAVE_IOS_BROWSER_BRAVE_SHIELDS_ELEMENT_PICKER_ELEMENT_PICKER_TAB_HELPER_H_

#import <Foundation/Foundation.h>

#include <string>

#include "base/functional/callback_forward.h"
#include "ios/web/public/web_state_user_data.h"

class GURL;
@class ElementPickerLocalizedTexts;
@protocol ElementPickerTabHelperBridge;

namespace web {
class WebState;
}

class ElementPickerTabHelper
    : public web::WebStateUserData<ElementPickerTabHelper> {
 public:
  ~ElementPickerTabHelper() override;

  void SetBridge(id<ElementPickerTabHelperBridge> bridge);

  // Adds a cosmetic filter hiding `selector` on `url`'s host. `url` is the
  // origin URL of the frame the element was picked in, which scopes the rule.
  // No-op when no bridge is attached.
  void AddCosmeticFilter(const std::string& selector, const GURL& url);

  // Opens the custom filters editor. No-op when no bridge is attached.
  void ManageCustomFilters();

  // Replies with whether dark mode is enabled and the background color as an
  // RGB value. Replies (NO, 0) when no bridge is attached.
  void ThemeInfo(base::OnceCallback<void(BOOL is_dark_mode_enabled,
                                         int32_t background_color)> callback);

  // `texts` is nil when the labels could not be resolved (e.g. no bridge is
  // attached).
  void LocalizedTexts(
      base::OnceCallback<void(ElementPickerLocalizedTexts* texts)> callback);

 private:
  friend class web::WebStateUserData<ElementPickerTabHelper>;

  explicit ElementPickerTabHelper(web::WebState* web_state);

  __weak id<ElementPickerTabHelperBridge> bridge_;
};

#endif  // BRAVE_IOS_BROWSER_BRAVE_SHIELDS_ELEMENT_PICKER_ELEMENT_PICKER_TAB_HELPER_H_
