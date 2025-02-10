// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_ANDROID_TAB_FEATURES_ANDROID_H_
#define BRAVE_BROWSER_ANDROID_TAB_FEATURES_ANDROID_H_

#include <memory>

#include "chrome/browser/android/tab_features_android.h"

namespace ai_chat {
class TabDataWebContentsObserver;
}

namespace content {
class WebContents;
}  // namespace content

class Profile;

class TabFeaturesAndroid : public TabFeaturesAndroid_Chromium {
 public:
  TabFeaturesAndroid(content::WebContents* web_contents, Profile* profile);
  ~TabFeaturesAndroid();

 private:
  std::unique_ptr<ai_chat::TabDataWebContentsObserver> tab_data_observer_;
};

#endif  // BRAVE_BROWSER_ANDROID_TAB_FEATURES_ANDROID_H_
