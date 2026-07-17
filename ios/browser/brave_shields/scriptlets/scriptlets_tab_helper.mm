// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/brave_shields/scriptlets/scriptlets_tab_helper.h"

#include <string>
#include <utility>
#include <vector>

#include "base/functional/callback.h"
#include "base/functional/callback_helpers.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/ios/browser/brave_shields/scriptlets/scriptlets_tab_helper_bridge.h"
#include "ios/web/public/web_state.h"
#include "net/base/apple/url_conversions.h"
#include "url/gurl.h"

ScriptletsTabHelper::ScriptletsTabHelper(web::WebState* web_state) {}

ScriptletsTabHelper::~ScriptletsTabHelper() = default;

void ScriptletsTabHelper::SetBridge(id<ScriptletsTabHelperBridge> bridge) {
  bridge_ = bridge;
}

void ScriptletsTabHelper::RequestScriptlets(
    const GURL& frame_url,
    base::OnceCallback<void(std::vector<std::string>)> callback) {
  if (!bridge_) {
    std::move(callback).Run({});
    return;
  }

  [bridge_
      requestScriptletsForFrameURL:net::NSURLWithGURL(frame_url)
                        completion:
                            base::CallbackToBlock(base::BindOnce(
                                [](base::OnceCallback<void(
                                       std::vector<std::string>)> callback,
                                   NSArray<NSString*>* scriptlets) {
                                  std::vector<std::string> result;
                                  result.reserve(scriptlets.count);
                                  for (NSString* scriptlet in scriptlets) {
                                    result.push_back(
                                        base::SysNSStringToUTF8(scriptlet));
                                  }
                                  std::move(callback).Run(std::move(result));
                                },
                                std::move(callback)))];
}
