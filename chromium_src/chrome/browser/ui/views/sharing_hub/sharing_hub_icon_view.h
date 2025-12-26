// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SHARING_HUB_SHARING_HUB_ICON_VIEW_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SHARING_HUB_SHARING_HUB_ICON_VIEW_H_

#include <memory>

#include "components/prefs/pref_member.h"

// Rename the class to avoid conflicts with the Chromium implementation
#define SharingHubIconView SharingHubIconView_ChromiumImpl

// Exposes private method to the Brave implementation.
#define GetController()   \
  GetController_Unused(); \
                          \
 protected:               \
  SharingHubBubbleController* GetController()

#include <chrome/browser/ui/views/sharing_hub/sharing_hub_icon_view.h>  // IWYU pragma: export

#undef GetController
#undef SharingHubIconView

namespace sharing_hub {

class SharingHubIconView : public SharingHubIconView_ChromiumImpl {
  METADATA_HEADER(SharingHubIconView, SharingHubIconView_ChromiumImpl)

 public:
  SharingHubIconView(CommandUpdater* command_updater,
                     IconLabelBubbleView::Delegate* icon_label_bubble_delegate,
                     PageActionIconView::Delegate* page_action_icon_delegate);
  ~SharingHubIconView() override;

  // SharingHubIconView_ChromiumImpl:
  void UpdateImpl() override;

 private:
  // Note that we can't BooleanPrefMember in constructor because controller is
  // not available yet. We need to initialize it in UpdateImpl().
  std::unique_ptr<BooleanPrefMember> pin_share_menu_button_pref_member_;
};

}  // namespace sharing_hub

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_VIEWS_SHARING_HUB_SHARING_HUB_ICON_VIEW_H_
