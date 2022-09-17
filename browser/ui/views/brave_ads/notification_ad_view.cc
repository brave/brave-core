/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_ads/notification_ad_view.h"

#include "brave/browser/ui/brave_ads/notification_ad_delegate.h"
#include "brave/browser/ui/brave_ads/notification_ad_popup_handler.h"
#include "brave/browser/ui/views/brave_ads/bounds_util.h"
#include "brave/browser/ui/views/brave_ads/notification_ad_popup.h"
#include "brave/grit/brave_generated_resources.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/compositor/layer.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/geometry/vector2d.h"
#include "ui/views/view.h"

namespace brave_ads {

NotificationAdView::NotificationAdView(const NotificationAd& notification_ad)
    : notification_ad_(notification_ad) {
  CreateView();
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

void NotificationAdView::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  node_data->role = ax::mojom::Role::kGenericContainer;
  node_data->AddStringAttribute(
      ax::mojom::StringAttribute::kRoleDescription,
      l10n_util::GetStringUTF8(IDS_BRAVE_ADS_NOTIFICATION_AD_ACCESSIBLE_NAME));

  if (accessible_name_.empty()) {
    node_data->SetNameFrom(ax::mojom::NameFrom::kAttributeExplicitlyEmpty);
  }

  node_data->SetName(accessible_name_);
}

bool NotificationAdView::OnMousePressed(const ui::MouseEvent& event) {
  initial_mouse_pressed_location_ = event.location();

  return true;
}

bool NotificationAdView::OnMouseDragged(const ui::MouseEvent& event) {
  const gfx::Vector2d movement =
      event.location() - initial_mouse_pressed_location_;

  if (!is_dragging_ && ExceededDragThreshold(movement)) {
    is_dragging_ = true;
  }

  if (!is_dragging_) {
    return false;
  }

  NotificationAdPopupHandler::Move(notification_ad_.id(), movement);

  return true;
}

void NotificationAdView::OnMouseReleased(const ui::MouseEvent& event) {
  if (is_dragging_) {
    is_dragging_ = false;
    return;
  }

  if (!event.IsOnlyLeftMouseButton()) {
    return;
  }

  NotificationAdDelegate* delegate = notification_ad_.delegate();
  if (delegate) {
    // This call will eventually lead to NotificationAdPopupHandler::Close call.
    delegate->OnClick();
  }

  View::OnMouseReleased(event);
}

void NotificationAdView::OnDeviceScaleFactorChanged(
    float old_device_scale_factor,
    float new_device_scale_factor) {
  GetWidget()->DeviceScaleFactorChanged(old_device_scale_factor,
                                        new_device_scale_factor);
}

void NotificationAdView::OnThemeChanged() {
  views::View::OnThemeChanged();

  SchedulePaint();
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

  NotifyAccessibilityEvent(ax::mojom::Event::kTextChanged, true);
}

BEGIN_METADATA(NotificationAdView, views::View)
END_METADATA

}  // namespace brave_ads
