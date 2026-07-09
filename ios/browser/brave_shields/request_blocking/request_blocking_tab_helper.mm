// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_shields/request_blocking/request_blocking_tab_helper.h"

#include <utility>

#include "base/functional/callback.h"
#include "base/functional/callback_helpers.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/ios/browser/brave_shields/request_blocking/request_blocking_tab_helper_bridge.h"
#include "ios/web/public/web_state.h"
#include "net/base/apple/url_conversions.h"
#include "url/gurl.h"

RequestBlockingTabHelper::RequestBlockingTabHelper(web::WebState* web_state) {}

RequestBlockingTabHelper::~RequestBlockingTabHelper() = default;

void RequestBlockingTabHelper::SetBridge(
    id<RequestBlockingTabHelperBridge> bridge) {
  bridge_ = bridge;
}

void RequestBlockingTabHelper::ShouldBlock(
    const GURL& request_url,
    const GURL& source_url,
    const std::string& resource_type,
    base::OnceCallback<void(bool)> callback) {
  if (!bridge_) {
    std::move(callback).Run(false);
    return;
  }

  [bridge_ shouldBlockRequestURL:net::NSURLWithGURL(request_url)
                       sourceURL:net::NSURLWithGURL(source_url)
                    resourceType:base::SysUTF8ToNSString(resource_type)
                      completion:base::CallbackToBlock(std::move(callback))];
}
