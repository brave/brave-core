/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_ads/notification_ad_view.h"

#include "brave/browser/ui/brave_ads/notification_ad_popup_handler.h"
#include "brave/grit/brave_generated_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/compositor/layer.h"
#include "ui/views/accessibility/view_accessibility.h"
#include "ui/views/view.h"
#include "ui/views/widget/widget.h"

namespace brave_ads {

NotificationAdView::NotificationAdView(const NotificationAd& notification_ad)
    : notification_ad_(notification_ad) {
  CreateView();

  GetViewAccessibility().SetRole(ax::mojom::Role::kAlertDialog);
  GetViewAccessibility().SetRoleDescription(
      l10n_util::GetStringUTF8(IDS_BRAVE_ADS_NOTIFICATION_AD_ACCESSIBLE_NAME));
}

NotificationAdView::~NotificationAdView() = default;

void NotificationAdView::UpdateContents(const NotificationAd& notification_ad) {
  notification_ad_ = notification_ad;

  SchedulePaint();

  MaybeNotifyAccessibilityEvent();
}

void NotificationAdView::OnCloseButtonPressed() {
  if (is_closing_) {
    return;
  }

  is_closing_ = true;

  NotificationAdPopupHandler::Close(notification_ad_.id(), /*by_user*/ true);
}

void NotificationAdView::OnDeviceScaleFactorChanged(
    float old_device_scale_factor,
    float new_device_scale_factor) {
  GetWidget()->DeviceScaleFactorChanged(old_device_scale_factor,
                                        new_device_scale_factor);
}

///////////////////////////////////////////////////////////////////////////////

void NotificationAdView::CreateView() {
  SetFocusBehavior(FocusBehavior::ALWAYS);

  // Paint to a dedicated layer to make the layer non-opaque
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);

  UpdateContents(notification_ad_);
}

void NotificationAdView::MaybeNotifyAccessibilityEvent() {
  const std::u16string accessible_name = notification_ad_.accessible_name();
  if (accessible_name == accessible_name_) {
    return;
  }

  accessible_name_ = accessible_name;
  UpdateAccessibleName();
}

void NotificationAdView::UpdateAccessibleName() {
  if (accessible_name_.empty()) {
    GetViewAccessibility().SetName(
        "", ax::mojom::NameFrom::kAttributeExplicitlyEmpty);
  } else {
    GetViewAccessibility().SetName(accessible_name_);
  }
}

BEGIN_METADATA(NotificationAdView)
END_METADATA

}  // namespace brave_ads
