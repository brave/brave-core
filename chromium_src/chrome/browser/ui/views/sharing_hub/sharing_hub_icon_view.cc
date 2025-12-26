// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/views/sharing_hub/sharing_hub_icon_view.h"

#include "base/functional/bind.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/sharing_hub/sharing_hub_bubble_controller_desktop_impl.h"
#include "chrome/common/pref_names.h"

#define SharingHubIconView SharingHubIconView_ChromiumImpl

#include <chrome/browser/ui/views/sharing_hub/sharing_hub_icon_view.cc>

#undef SharingHubIconView

namespace sharing_hub {

SharingHubIconView::SharingHubIconView(
    CommandUpdater* command_updater,
    IconLabelBubbleView::Delegate* icon_label_bubble_delegate,
    PageActionIconView::Delegate* page_action_icon_delegate)
    : SharingHubIconView_ChromiumImpl(command_updater,
                                      icon_label_bubble_delegate,
                                      page_action_icon_delegate) {}

SharingHubIconView::~SharingHubIconView() = default;

void SharingHubIconView::UpdateImpl() {
  if (!pin_share_menu_button_pref_member_ && GetController()) {
    pin_share_menu_button_pref_member_ = std::make_unique<BooleanPrefMember>();
    pin_share_menu_button_pref_member_->Init(
        prefs::kPinShareMenuButton,
        static_cast<SharingHubBubbleControllerDesktopImpl*>(GetController())
            ->GetProfile()
            ->GetPrefs(),
        base::BindRepeating(&SharingHubIconView::UpdateImpl,
                            base::Unretained(this)));
  }

  SharingHubIconView_ChromiumImpl::UpdateImpl();
}

BEGIN_METADATA(SharingHubIconView)
END_METADATA

}  // namespace sharing_hub
