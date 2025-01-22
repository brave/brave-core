// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_ANDROID_BRAVE_TAB_FEATURES_ANDROID_H_
#define BRAVE_BROWSER_ANDROID_BRAVE_TAB_FEATURES_ANDROID_H_

#include "chrome/browser/android/tab_features_android.h"

namespace content {
class WebContents;
}  // namespace content

class Profile;

class BraveTabFeaturesAndroid : public TabFeaturesAndroid {
 public:
  BraveTabFeaturesAndroid(content::WebContents* web_contents, Profile* profile);
  ~BraveTabFeaturesAndroid() override;
};

#endif  // BRAVE_BROWSER_ANDROID_BRAVE_TAB_FEATURES_ANDROID_H_
