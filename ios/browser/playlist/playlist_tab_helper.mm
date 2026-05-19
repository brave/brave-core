// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/playlist/playlist_tab_helper.h"

#import <Foundation/Foundation.h>

#include <optional>
#include <string>

#include "base/json/json_writer.h"
#include "base/strings/sys_string_conversions.h"
#include "brave/ios/browser/playlist/playlist_tab_helper_bridge.h"

namespace playlist {

PlaylistTabHelper::PlaylistTabHelper(web::WebState* web_state) {}

PlaylistTabHelper::~PlaylistTabHelper() = default;

void PlaylistTabHelper::SetBridge(id<PlaylistTabHelperBridge> bridge) {
  bridge_ = bridge;
}

void PlaylistTabHelper::OnMediaDetected(const base::DictValue& item) {
  if (!bridge_) {
    return;
  }

  // The bridge consumes the item as an NSDictionary so it can be decoded into a
  // `PlaylistInfo` on the Swift side. Round-trip through JSON to convert the
  // base::Value into Foundation objects.
  std::optional<std::string> json = base::WriteJson(item);
  if (!json) {
    return;
  }

  NSData* data =
      [base::SysUTF8ToNSString(*json) dataUsingEncoding:NSUTF8StringEncoding];
  id object = [NSJSONSerialization JSONObjectWithData:data options:0 error:nil];
  if (![object isKindOfClass:[NSDictionary class]]) {
    return;
  }

  [bridge_ onMediaDetected:object];
}

}  // namespace playlist
