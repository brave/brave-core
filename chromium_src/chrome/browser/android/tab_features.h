// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_ANDROID_TAB_FEATURES_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_ANDROID_TAB_FEATURES_H_

#define TabFeatures TabFeatures_Chromium

#include <chrome/browser/android/tab_features.h>  // IWYU pragma: export

#undef TabFeatures

namespace tabs {
class BraveTabFeatures;
class TabFeatures {
 public:
  TabFeatures(content::WebContents* web_contents, Profile* profile);
  ~TabFeatures();

  NewTabPagePreloadPipelineManager* new_tab_page_preload_pipeline_manager();

 private:
  friend BraveTabFeatures;
  std::unique_ptr<BraveTabFeatures> brave_tab_features_;
};
}  // namespace tabs

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_ANDROID_TAB_FEATURES_H_
