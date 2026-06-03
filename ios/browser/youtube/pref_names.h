// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_IOS_BROWSER_YOUTUBE_PREF_NAMES_H_
#define BRAVE_IOS_BROWSER_YOUTUBE_PREF_NAMES_H_

namespace youtube {

enum class AutoQualityMode {
  // Video quality will remain automatic when visiting YouTube video pages
  kOff = 0,
  // YouTube video quality will always be set to the highest quality
  kOn = 1,
  // YouTube video quality will only ever be set to the highest quality if the
  // user is on a wifi network. If the user drops to a cellular network while
  // on a page it will update the quality back to `auto`
  kWifiOnly = 2
};

namespace prefs {

inline constexpr char kAutoQualityMode[] = "youtube_auto_quality_mode";

}

}  // namespace youtube

#endif  // BRAVE_IOS_BROWSER_YOUTUBE_PREF_NAMES_H_
