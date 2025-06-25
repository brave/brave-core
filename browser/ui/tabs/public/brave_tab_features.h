// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_TABS_PUBLIC_BRAVE_TAB_FEATURES_H_
#define BRAVE_BROWSER_UI_TABS_PUBLIC_BRAVE_TAB_FEATURES_H_

#include <memory>

#include "chrome/browser/ui/tabs/public/tab_features.h"

class Profile;

namespace ai_chat {
class TabDataWebContentsObserver;
}

namespace tabs {

class TabInterface;

class BraveTabFeatures : public TabFeatures {
 public:
  static BraveTabFeatures* FromTabFeatures(TabFeatures* tab_features);
  ~BraveTabFeatures() override;

  void Init(TabInterface& tab, Profile* profile) override;

 protected:
  friend TabFeatures;
  BraveTabFeatures();

 private:
  std::unique_ptr<ai_chat::TabDataWebContentsObserver> tab_data_observer_;
};

}  // namespace tabs
#endif  // BRAVE_BROWSER_UI_TABS_PUBLIC_BRAVE_TAB_FEATURES_H_
