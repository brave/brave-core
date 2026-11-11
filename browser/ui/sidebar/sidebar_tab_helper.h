/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_TAB_HELPER_H_
#define BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_TAB_HELPER_H_

#include <memory>

#include "chrome/browser/ui/tabs/contents_observing_tab_feature.h"

namespace tabs {
class TabInterface;
}  // namespace tabs

namespace sidebar {

// Helper to launch the Leo panel one time.
class SidebarTabHelper : public tabs::ContentsObservingTabFeature {
 public:
  // Returns nullptr unless the one-shot Leo panel can still be shown for
  // `tab`.
  static std::unique_ptr<SidebarTabHelper> MaybeCreate(tabs::TabInterface& tab);

  explicit SidebarTabHelper(tabs::TabInterface& tab);
  ~SidebarTabHelper() override;

  SidebarTabHelper(const SidebarTabHelper&) = delete;
  SidebarTabHelper& operator=(const SidebarTabHelper&) = delete;

 private:
  // content::WebContentsObserver:
  void PrimaryPageChanged(content::Page& page) override;
};

}  // namespace sidebar

#endif  // BRAVE_BROWSER_UI_SIDEBAR_SIDEBAR_TAB_HELPER_H_
