/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/toolbar/bookmark_button.h"

#include "base/strings/utf_string_conversions.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/ui/view_ids.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/grit/generated_resources.h"
#include "components/strings/grit/components_strings.h"
#include "components/omnibox/browser/vector_icons.h"
#include "ui/accessibility/ax_enums.mojom.h"
#include "ui/accessibility/ax_node_data.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/theme_provider.h"
#include "ui/gfx/paint_vector_icon.h"

BookmarkButton::BookmarkButton(views::ButtonListener* listener)
    : ToolbarButton(listener) {
  SetID(VIEW_ID_STAR_BUTTON);
  set_tag(IDC_BOOKMARK_THIS_TAB);
  SetAccessibleName(l10n_util::GetStringUTF16(IDS_ACCNAME_FORWARD));
}

BookmarkButton::~BookmarkButton() {
}

const char* BookmarkButton::GetClassName() const {
  return "BookmarkButton";
}

base::string16 BookmarkButton::GetTooltipText(const gfx::Point& p) const {
  int textId = active_ ? IDS_TOOLTIP_STARRED : IDS_TOOLTIP_STAR;
  return l10n_util::GetStringUTF16(textId);
}

void BookmarkButton::GetAccessibleNodeData(ui::AXNodeData* node_data) {
  int textId = active_ ? IDS_TOOLTIP_STARRED : IDS_TOOLTIP_STAR;
    node_data->role = ax::mojom::Role::kButton;
  node_data->SetName(l10n_util::GetStringUTF16(textId));
}

void BookmarkButton::SetToggled(bool on) {
  active_ = on;
  UpdateImage();
}

void BookmarkButton::UpdateImage() {
  const ui::ThemeProvider* tp = GetThemeProvider();

  SkColor icon_color = tp->GetColor(ThemeProperties::COLOR_TOOLBAR_BUTTON_ICON);
  const gfx::VectorIcon& icon =
      active_ ? omnibox::kStarActiveIcon : omnibox::kStarIcon;
  SetImage(views::Button::STATE_NORMAL,
           gfx::CreateVectorIcon(icon, icon_color));
}
