/* Copyright 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/toolbar/speedreader_button.h"

#include "base/strings/utf_string_conversions.h"
#include "brave/app/brave_command_ids.h"
#include "brave/app/vector_icons/vector_icons.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/view_ids.h"
#include "chrome/grit/generated_resources.h"
#include "components/grit/brave_components_strings.h"
#include "components/strings/grit/components_strings.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/theme_provider.h"
#include "ui/gfx/paint_vector_icon.h"

SpeedreaderButton::SpeedreaderButton(views::ButtonListener* listener, bool on)
    : ToolbarButton(listener), on_(on) {
  SetID(BRAVE_VIEW_ID_SPEEDREADER_BUTTON);
  set_tag(IDC_TOGGLE_SPEEDREADER);
  SetAccessibleName(l10n_util::GetStringUTF16(IDS_ACCNAME_FORWARD));
}

SpeedreaderButton::~SpeedreaderButton() {}

const char* SpeedreaderButton::GetClassName() const {
  return "SpeedreaderButton";
}

base::string16 SpeedreaderButton::GetTooltipText(const gfx::Point& p) const {
  int textId =
      on_ ? IDS_TOOLTIP_TURN_OFF_SPEEDREADER : IDS_TOOLTIP_TURN_ON_SPEEDREADER;
  return l10n_util::GetStringUTF16(textId);
}

void SpeedreaderButton::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  int textId =
      on_ ? IDS_TOOLTIP_TURN_OFF_SPEEDREADER : IDS_TOOLTIP_TURN_ON_SPEEDREADER;
  node_data->role = ax::mojom::Role::kButton;
  node_data->SetName(l10n_util::GetStringUTF16(textId));
}

void SpeedreaderButton::SetHighlighted(bool bubble_visible) {
  AnimateInkDrop(bubble_visible ? views::InkDropState::ACTIVATED
                                : views::InkDropState::DEACTIVATED,
                 nullptr);
}

void SpeedreaderButton::Toggle() {
  on_ = !on_;
  UpdateImage();
}

void SpeedreaderButton::SetActive(bool active) {
  active_ = active;
  UpdateImage();
}

void SpeedreaderButton::UpdateImage() {
  const ui::ThemeProvider* tp = GetThemeProvider();

  SkColor icon_color = tp->GetColor(ThemeProperties::COLOR_TOOLBAR_BUTTON_ICON);
  const gfx::VectorIcon& icon =
      on_ ? (active_ ? kSpeedreaderOnActiveIcon : kSpeedreaderOnInactiveIcon)
          : kSpeedreaderIcon;
  SetImage(views::Button::STATE_NORMAL,
           gfx::CreateVectorIcon(icon, icon_color));
}
