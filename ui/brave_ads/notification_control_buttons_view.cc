/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/ui/brave_ads/notification_control_buttons_view.h"

#include <memory>

#include "brave/app/vector_icons/vector_icons.h"
#include "brave/grit/brave_theme_resources.h"
#include "brave/ui/brave_ads/notification_view.h"
#include "brave/ui/brave_ads/padded_button.h"
#include "brave/ui/brave_ads/padded_image.h"
#include "brave/ui/brave_ads/public/cpp/constants.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/compositor/layer.h"
#include "ui/events/event.h"
#include "ui/gfx/color_palette.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/strings/grit/ui_strings.h"
#include "ui/views/background.h"
#include "ui/views/controls/image_view.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/layout/box_layout.h"

namespace brave_ads {

const char NotificationControlButtonsView::kViewClassName[] =
    "NotificationControlButtonsView";

NotificationControlButtonsView::NotificationControlButtonsView(
    NotificationView* message_view)
    : message_view_(message_view) {
  DCHECK(message_view);
  SetLayoutManager(std::make_unique<views::BoxLayout>(
      views::BoxLayout::Orientation::kHorizontal));

  // Use layer to change the opacity.
  SetPaintToLayer();
  layer()->SetFillsBoundsOpaquely(false);

  SetBackground(views::CreateSolidBackground(kControlButtonBackgroundColor));
}

NotificationControlButtonsView::~NotificationControlButtonsView() = default;

void NotificationControlButtonsView::ShowInfoButton(bool show) {
  if (show && !info_button_) {
    // Add the button next right to the snooze button.
    // TODO(Albert Wang): https://github.com/brave/brave-browser/issues/11798
    info_button_ = std::make_unique<PaddedImage>();
    info_button_->set_owned_by_client();
    info_button_->SetImage(
        gfx::CreateVectorIcon(kBraveAdsInfoIcon, 35, SK_ColorTRANSPARENT));
    AddChildView(info_button_.get());
    Layout();
  } else if (!show && info_button_) {
    DCHECK(Contains(info_button_.get()));
    info_button_.reset();
  }
}

void NotificationControlButtonsView::ShowCloseButton(bool show) {
  if (show && !close_button_) {
    close_button_ = std::make_unique<PaddedButton>(
        base::BindRepeating(&NotificationView::OnCloseButtonPressed,
                            base::Unretained(message_view_)));
    close_button_->set_owned_by_client();
    close_button_->SetImage(
        views::Button::STATE_NORMAL,
        gfx::CreateVectorIcon(
            kBraveAdsCloseButtonIcon,
            18,
            kBraveAdsCloseButtonIconColor));

    // Add the button at the last.
    AddChildView(close_button_.get());
    Layout();
  } else if (!show && close_button_) {
    DCHECK(Contains(close_button_.get()));
    close_button_.reset();
  }
}

void NotificationControlButtonsView::ShowButtons(bool show) {
  DCHECK(layer());
  // Manipulate the opacity instead of changing the visibility to keep the tab
  // order even when the view is invisible.
  layer()->SetOpacity(show ? 1. : 0.);
  SetCanProcessEventsWithinSubtree(show);
}

bool NotificationControlButtonsView::IsAnyButtonFocused() const {
  return (close_button_ && close_button_->HasFocus());
}

views::Button* NotificationControlButtonsView::close_button() const {
  return close_button_.get();
}

views::ImageView* NotificationControlButtonsView::info_button() const {
  return info_button_.get();
}

const char* NotificationControlButtonsView::GetClassName() const {
  return kViewClassName;
}

}  // namespace brave_ads
