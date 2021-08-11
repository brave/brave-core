/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/brave_ads/ad_notification_view.h"

#include <string>

#include "brave/browser/ui/views/brave_ads/ad_notification_popup.h"
#include "brave/browser/ui/views/brave_ads/bounds_util.h"
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

AdNotificationView::AdNotificationView(const AdNotification& ad_notification)
    : ad_notification_(ad_notification) {
  CreateView();
}

AdNotificationView::~AdNotificationView() = default;

void AdNotificationView::UpdateContents(const AdNotification& ad_notification) {
  ad_notification_ = ad_notification;

  SchedulePaint();

  MaybeNotifyAccessibilityEvent();
}

void AdNotificationView::OnCloseButtonPressed() {
  if (is_closing_) {
    return;
  }

  is_closing_ = true;

  const std::string id = ad_notification_.id();
  AdNotificationPopup::Close(id, /* by_user */ true);
}

void AdNotificationView::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  node_data->role = ax::mojom::Role::kGenericContainer;
  node_data->AddStringAttribute(
      ax::mojom::StringAttribute::kRoleDescription,
      l10n_util::GetStringUTF8(IDS_BRAVE_ADS_AD_NOTIFICATION_ACCESSIBLE_NAME));

  if (accessible_name_.empty()) {
    node_data->SetNameFrom(ax::mojom::NameFrom::kAttributeExplicitlyEmpty);
  }

  node_data->SetName(accessible_name_);
}

bool AdNotificationView::OnMousePressed(const ui::MouseEvent& event) {
  initial_mouse_pressed_location_ = event.location();

  return true;
}

bool AdNotificationView::OnMouseDragged(const ui::MouseEvent& event) {
  const gfx::Vector2d movement =
      event.location() - initial_mouse_pressed_location_;

  if (!is_dragging_ && ExceededDragThreshold(movement)) {
    is_dragging_ = true;
  }

  if (!is_dragging_) {
    return false;
  }

  const std::string id = ad_notification_.id();
  gfx::Rect bounds = AdNotificationPopup::GetBounds(id) + movement;
  const gfx::NativeView native_view = GetWidget()->GetNativeView();
  AdjustBoundsToFitWorkAreaForNativeView(&bounds, native_view);
  GetWidget()->SetBounds(bounds);

  return true;
}

void AdNotificationView::OnMouseReleased(const ui::MouseEvent& event) {
  if (is_dragging_) {
    is_dragging_ = false;
    return;
  }

  if (!event.IsOnlyLeftMouseButton()) {
    return;
  }

  const std::string id = ad_notification_.id();
  AdNotificationPopup::OnClick(id);

  View::OnMouseReleased(event);
}

void AdNotificationView::OnDeviceScaleFactorChanged(
    float old_device_scale_factor,
    float new_device_scale_factor) {
  GetWidget()->DeviceScaleFactorChanged(old_device_scale_factor,
                                        new_device_scale_factor);
}

void AdNotificationView::OnThemeChanged() {
  views::View::OnThemeChanged();

  SchedulePaint();
}

///////////////////////////////////////////////////////////////////////////////

void AdNotificationView::CreateView() {
  SetFocusBehavior(FocusBehavior::ALWAYS);

  // Paint to a dedicated layer to make the layer non-opaque
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);

  UpdateContents(ad_notification_);
}

void AdNotificationView::MaybeNotifyAccessibilityEvent() {
  const std::u16string accessible_name = ad_notification_.accessible_name();
  if (accessible_name == accessible_name_) {
    return;
  }

  accessible_name_ = accessible_name;

  NotifyAccessibilityEvent(ax::mojom::Event::kTextChanged, true);
}

BEGIN_METADATA(AdNotificationView, views::View)
END_METADATA

}  // namespace brave_ads
