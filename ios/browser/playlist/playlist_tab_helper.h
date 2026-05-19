// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_PLAYLIST_PLAYLIST_TAB_HELPER_H_
#define BRAVE_IOS_BROWSER_PLAYLIST_PLAYLIST_TAB_HELPER_H_

#include "base/values.h"
#include "ios/web/public/web_state_user_data.h"

@protocol PlaylistTabHelperBridge;

namespace web {
class WebState;
}  // namespace web

namespace playlist {

// Holds the bridge to the Swift `PlaylistTabHelper` and forwards media detected
// by `PlaylistJavaScriptFeature` to it. Mirrors the per-WebState ownership used
// by other Brave tab helpers (e.g. BraveTalkTabHelper).
class PlaylistTabHelper : public web::WebStateUserData<PlaylistTabHelper> {
 public:
  ~PlaylistTabHelper() override;

  void SetBridge(id<PlaylistTabHelperBridge> bridge);

  // Forwards a detected media item to the registered bridge, if any. `item` is
  // the `PlaylistItem` payload produced by playlist_utils.ts.
  void OnMediaDetected(const base::DictValue& item);

 private:
  friend class web::WebStateUserData<PlaylistTabHelper>;

  explicit PlaylistTabHelper(web::WebState* web_state);

  __weak id<PlaylistTabHelperBridge> bridge_;
};

}  // namespace playlist

#endif  // BRAVE_IOS_BROWSER_PLAYLIST_PLAYLIST_TAB_HELPER_H_
