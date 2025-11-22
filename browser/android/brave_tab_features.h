// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_ANDROID_BRAVE_TAB_FEATURES_H_
#define BRAVE_BROWSER_ANDROID_BRAVE_TAB_FEATURES_H_

#include <memory>

#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "chrome/browser/android/tab_features.h"

class Profile;

#if BUILDFLAG(ENABLE_AI_CHAT)
namespace ai_chat {
class TabDataWebContentsObserver;
}
#endif

namespace content {
class WebContents;
}  // namespace content

namespace tabs {

// This class holds state that is scoped to a tab in Android. It is constructed
// after the WebContents/tab_helpers, and destroyed before.
class BraveTabFeatures : public TabFeatures_Chromium {
 public:
  BraveTabFeatures(content::WebContents* web_contents, Profile* profile);
  ~BraveTabFeatures();
  static BraveTabFeatures* FromTabFeatures(TabFeatures* tab_features);

 private:
#if BUILDFLAG(ENABLE_AI_CHAT)
  std::unique_ptr<ai_chat::TabDataWebContentsObserver> tab_data_observer_;
#endif
};

}  // namespace tabs

#endif  // BRAVE_BROWSER_ANDROID_BRAVE_TAB_FEATURES_H_
