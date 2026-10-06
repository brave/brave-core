// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_BRAVE_SHIELDS_ELEMENT_PICKER_ELEMENT_PICKER_TAB_HELPER_H_
#define BRAVE_IOS_BROWSER_BRAVE_SHIELDS_ELEMENT_PICKER_ELEMENT_PICKER_TAB_HELPER_H_

#import <Foundation/Foundation.h>

#include <string>

#include "base/functional/callback_forward.h"
#include "brave/ios/browser/brave_shields/element_picker/element_picker_localized_texts.h"
#include "ios/web/public/web_state_user_data.h"

class GURL;
@protocol ElementPickerTabHelperBridge;

namespace web {
class WebState;
}

class ElementPickerTabHelper
    : public web::WebStateUserData<ElementPickerTabHelper> {
 public:
  ~ElementPickerTabHelper() override;

  void SetBridge(id<ElementPickerTabHelperBridge> bridge);

  void AddCosmeticFilter(const std::string& selector, const GURL& url);

  void ManageCustomFilters();

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
