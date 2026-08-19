// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_shields/cosmetic_filtering/cosmetic_filtering_tab_helper.h"

#include <string>
#include <utility>

#include "base/containers/span.h"
#include "base/functional/callback.h"
#include "base/functional/callback_helpers.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/ios/browser/brave_shields/cosmetic_filtering/cosmetic_filtering_tab_helper_bridge.h"
#include "ios/web/public/web_state.h"
#include "net/base/apple/url_conversions.h"
#include "url/gurl.h"

namespace {

NSSet<NSString*>* NSSetFromStrings(base::span<const std::string> strings) {
  NSMutableSet<NSString*>* set = [NSMutableSet setWithCapacity:strings.size()];
  for (const std::string& string : strings) {
    [set addObject:base::SysUTF8ToNSString(string)];
  }
  return [set copy];
}

}  // namespace

CosmeticFilteringTabHelper::CosmeticFilteringTabHelper(
    web::WebState* web_state) {}

CosmeticFilteringTabHelper::~CosmeticFilteringTabHelper() = default;

void CosmeticFilteringTabHelper::SetBridge(
    id<CosmeticFilteringTabHelperBridge> bridge) {
  bridge_ = bridge;
}

void CosmeticFilteringTabHelper::CosmeticFilteringArgsFor(
    const GURL& url,
    base::OnceCallback<void(CosmeticFilteringArgs*)> callback) {
  if (!bridge_) {
    std::move(callback).Run(nil);
    return;
  }

  [bridge_ cosmeticFilteringArgsFor:net::NSURLWithGURL(url)
                         completion:base::CallbackToBlock(std::move(callback))];
}

void CosmeticFilteringTabHelper::SelectorsToHideFor(
    const GURL& url,
    base::span<const std::string> ids,
    base::span<const std::string> classes,
    base::OnceCallback<void(NSSet<NSString*>* standard_selectors,
                            NSSet<NSString*>* aggressive_selectors)> callback) {
  if (!bridge_) {
    std::move(callback).Run(nil, nil);
    return;
  }

  [bridge_ selectorsToHideFor:net::NSURLWithGURL(url)
                          ids:NSSetFromStrings(ids)
                      classes:NSSetFromStrings(classes)
                   completion:base::CallbackToBlock(std::move(callback))];
}
