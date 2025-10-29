// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_TABS_PUBLIC_BRAVE_TAB_FEATURES_H_
#define BRAVE_BROWSER_UI_TABS_PUBLIC_BRAVE_TAB_FEATURES_H_

#include <memory>

#include "brave/components/ai_chat/core/common/buildflags/buildflags.h"
#include "brave/components/psst/buildflags/buildflags.h"
#include "chrome/browser/ui/tabs/public/tab_features.h"

class Profile;

#if BUILDFLAG(ENABLE_AI_CHAT)
namespace ai_chat {
class TabDataWebContentsObserver;
}
#endif

#if BUILDFLAG(ENABLE_PSST)
namespace psst {
class PsstTabWebContentsObserver;
}
#endif

namespace tabs {

class TabInterface;

class BraveTabFeatures : public TabFeatures {
 public:
  static BraveTabFeatures* FromTabFeatures(TabFeatures* tab_features);
  BraveTabFeatures();
  ~BraveTabFeatures() override;

  void Init(TabInterface& tab, Profile* profile) override;

#if BUILDFLAG(ENABLE_PSST)
  psst::PsstTabWebContentsObserver* psst_web_contents_observer() {
    return psst_web_contents_observer_.get();
  }
#endif

 private:
#if BUILDFLAG(ENABLE_AI_CHAT)
  std::unique_ptr<ai_chat::TabDataWebContentsObserver> tab_data_observer_;
#endif
#if BUILDFLAG(ENABLE_PSST)
  std::unique_ptr<psst::PsstTabWebContentsObserver> psst_web_contents_observer_;
#endif
};

}  // namespace tabs
#endif  // BRAVE_BROWSER_UI_TABS_PUBLIC_BRAVE_TAB_FEATURES_H_
