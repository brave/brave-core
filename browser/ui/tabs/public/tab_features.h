// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_TABS_PUBLIC_TAB_FEATURES_H_
#define BRAVE_BROWSER_UI_TABS_PUBLIC_TAB_FEATURES_H_

#include <memory>

#include "brave/components/psst/buildflags/buildflags.h"
#include "chrome/browser/ui/tabs/public/tab_features.h"

class Profile;

namespace ai_chat {
class TabDataWebContentsObserver;
}

#if BUILDFLAG(ENABLE_PSST)
namespace psst {
class PsstTabWebContentsObserver;
}
#endif

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

#if BUILDFLAG(ENABLE_PSST)
  inline psst::PsstTabWebContentsObserver* psst_web_contents_observer();
#endif

 private:
  std::unique_ptr<ai_chat::TabDataWebContentsObserver> tab_data_observer_;
#if BUILDFLAG(ENABLE_PSST)
  std::unique_ptr<psst::PsstTabWebContentsObserver> psst_web_contents_observer_;
#endif
};

}  // namespace tabs
#endif  // BRAVE_BROWSER_UI_TABS_PUBLIC_TAB_FEATURES_H_
