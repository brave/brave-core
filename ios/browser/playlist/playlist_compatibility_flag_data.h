// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_PLAYLIST_PLAYLIST_COMPATIBILITY_FLAG_DATA_H_
#define BRAVE_IOS_BROWSER_PLAYLIST_PLAYLIST_COMPATIBILITY_FLAG_DATA_H_

#include "ios/web/public/web_state.h"
#include "ios/web/public/web_state_user_data.h"

namespace playlist {

// A piece of data that when stored on WebState indicates that the page should
// run in playlist compatibility mode.
class PlaylistCompatibilityFlagData
    : public web::WebStateUserData<PlaylistCompatibilityFlagData> {
 public:
  ~PlaylistCompatibilityFlagData() override;

 private:
  explicit PlaylistCompatibilityFlagData(web::WebState* web_state);
  friend class web::WebStateUserData<PlaylistCompatibilityFlagData>;
};

}  // namespace playlist

#endif  // BRAVE_IOS_BROWSER_PLAYLIST_PLAYLIST_COMPATIBILITY_FLAG_DATA_H_
