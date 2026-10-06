// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_shields/element_picker/element_picker_tab_helper.h"

#include <string>
#include <utility>

#include "base/functional/callback.h"
#include "base/functional/callback_helpers.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/ios/browser/brave_shields/element_picker/element_picker_tab_helper_bridge.h"
#include "ios/web/public/web_state.h"
#include "net/base/apple/url_conversions.h"
#include "url/gurl.h"

ElementPickerTabHelper::ElementPickerTabHelper(web::WebState* web_state) {}

ElementPickerTabHelper::~ElementPickerTabHelper() = default;

void ElementPickerTabHelper::SetBridge(
    id<ElementPickerTabHelperBridge> bridge) {
  bridge_ = bridge;
}

void ElementPickerTabHelper::AddCosmeticFilter(const std::string& selector,
                                               const GURL& url) {
  [bridge_ addCosmeticFilter:base::SysUTF8ToNSString(selector)
                      forURL:net::NSURLWithGURL(url)];
}

void ElementPickerTabHelper::ManageCustomFilters() {
  [bridge_ manageCustomFilters];
}

void ElementPickerTabHelper::ThemeInfo(
    base::OnceCallback<void(BOOL is_dark_mode_enabled,
                            int32_t background_color)> callback) {
  if (!bridge_) {
    std::move(callback).Run(NO, 0);
    return;
  }

  [bridge_ elementPickerThemeInfoWithCompletion:base::CallbackToBlock(
                                                    std::move(callback))];
}

void ElementPickerTabHelper::LocalizedTexts(
    base::OnceCallback<void(ElementPickerLocalizedTexts* texts)> callback) {
  if (!bridge_) {
    std::move(callback).Run(nil);
    return;
  }

  [bridge_ elementPickerLocalizedTextsWithCompletion:base::CallbackToBlock(
                                                         std::move(callback))];
}
