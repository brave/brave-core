// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_shields/cosmetic_filtering/cosmetic_filtering_tab_helper.h"

#include <string>
#include <utility>
#include <vector>

#include "base/functional/callback.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/ios/browser/brave_shields/cosmetic_filtering/cosmetic_filtering_tab_helper_bridge.h"
#include "ios/web/public/web_state.h"
#include "net/base/apple/url_conversions.h"
#include "url/gurl.h"

namespace {

NSSet<NSString*>* NSSetFromStrings(const std::vector<std::string>& strings) {
  NSMutableSet<NSString*>* set = [NSMutableSet setWithCapacity:strings.size()];
  for (const std::string& string : strings) {
    [set addObject:base::SysUTF8ToNSString(string)];
  }
  return set;
}

std::vector<std::string> StringsFromNSSet(NSSet<NSString*>* set) {
  std::vector<std::string> strings;
  strings.reserve(set.count);
  for (NSString* string in set) {
    strings.push_back(base::SysNSStringToUTF8(string));
  }
  return strings;
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
  }

  __block auto args_callback = std::move(callback);
  [bridge_ cosmeticFilteringArgsFor:net::NSURLWithGURL(url)
                         completion:^(CosmeticFilteringArgs* args) {
                           std::move(args_callback).Run(args);
                         }];
}

void CosmeticFilteringTabHelper::SelectorsToHideFor(
    const GURL& url,
    const std::vector<std::string>& ids,
    const std::vector<std::string>& classes,
    base::OnceCallback<
        void(const std::vector<std::string>* standard_selectors,
             const std::vector<std::string>* aggressive_selectors)> callback) {
  if (!bridge_) {
    std::move(callback).Run(nullptr, nullptr);
    return;
  }

  __block auto selectors_to_hide_callback = std::move(callback);
  [bridge_ selectorsToHideFor:net::NSURLWithGURL(url)
                          ids:NSSetFromStrings(ids)
                      classes:NSSetFromStrings(classes)
                   completion:^(NSSet<NSString*>* standard_selectors,
                                NSSet<NSString*>* aggressive_selectors) {
                     std::vector<std::string> standard =
                         StringsFromNSSet(standard_selectors);
                     std::vector<std::string> aggressive =
                         StringsFromNSSet(aggressive_selectors);
                     std::move(selectors_to_hide_callback)
                         .Run(&standard, &aggressive);
                   }];
}
