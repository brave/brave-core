// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_TABS_PUBLIC_TAB_FEATURES_H_
#define BRAVE_BROWSER_UI_TABS_PUBLIC_TAB_FEATURES_H_

#include <memory>

#include "chrome/browser/ui/tabs/public/tab_features.h"

class Profile;

namespace ai_chat {
class TabDataWebContentsObserver;
}

namespace brave_screenshots {
class BraveScreenshotsTabFeature;
}  // namespace brave_screenshots

namespace tabs {

class TabInterface;

class TabFeatures : public TabFeatures_Chromium {
 public:
  using TabFeaturesFactory =
      base::RepeatingCallback<std::unique_ptr<TabFeatures>()>;
  static std::unique_ptr<TabFeatures> CreateTabFeatures();
  static void ReplaceTabFeaturesForTesting(TabFeaturesFactory factory);

  TabFeatures();
  ~TabFeatures() override;

  void Init(TabInterface& tab, Profile* profile) override;

  // Brave Screenshots (via Context Menu and Commander)
  brave_screenshots::BraveScreenshotsTabFeature*
  brave_screenshots_tab_feature() {
    return brave_screenshots_tab_feature_.get();
  }

 private:
  // Brave Screenshots (via Context Menu and Commander)
  std::unique_ptr<brave_screenshots::BraveScreenshotsTabFeature>
      brave_screenshots_tab_feature_;
  std::unique_ptr<ai_chat::TabDataWebContentsObserver> tab_data_observer_;
};

}  // namespace tabs
#endif  // BRAVE_BROWSER_UI_TABS_PUBLIC_TAB_FEATURES_H_
