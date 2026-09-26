// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_ANDROID_BRAVE_TAB_FEATURES_H_
#define BRAVE_BROWSER_ANDROID_BRAVE_TAB_FEATURES_H_

#include <memory>

#include "chrome/browser/android/tab_features.h"

class Profile;

namespace ai_chat {
class TabDataWebContentsObserver;
class WebMcpInjector;
}  // namespace ai_chat

namespace content {
class WebContents;
}  // namespace content

namespace tabs {

class ContentsObservingTabFeature;

// This class holds state that is scoped to a tab in Android. It is constructed
// after the WebContents/tab_helpers, and destroyed before.
class BraveTabFeatures : public TabFeatures_Chromium {
 public:
  BraveTabFeatures(content::WebContents* web_contents, Profile* profile);
  ~BraveTabFeatures();
  static BraveTabFeatures* FromTabFeatures(TabFeatures* tab_features);

 private:
  // Records Cloudflare javascript-detection script loads. Null for
  // incognito/guest profiles and when captcha metrics are disabled.
  std::unique_ptr<ContentsObservingTabFeature>
      cloudflare_js_detection_tab_helper_;
  std::unique_ptr<ai_chat::TabDataWebContentsObserver> tab_data_observer_;
  std::unique_ptr<ai_chat::WebMcpInjector> web_mcp_injector_;
};

}  // namespace tabs

#endif  // BRAVE_BROWSER_ANDROID_BRAVE_TAB_FEATURES_H_
