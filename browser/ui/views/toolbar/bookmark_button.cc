/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/toolbar/bookmark_button.h"

#include <utility>

#include "base/strings/utf_string_conversions.h"
#include "brave/components/l10n/common/localization_util.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/ui/color/chrome_color_id.h"
#include "chrome/browser/ui/view_ids.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/grit/generated_resources.h"
#include "components/omnibox/browser/vector_icons.h"
#include "components/strings/grit/components_strings.h"
#include "ui/gfx/paint_vector_icon.h"

BookmarkButton::BookmarkButton(PressedCallback callback)
    : ToolbarButton(std::move(callback)) {
  SetID(VIEW_ID_STAR_BUTTON);
  set_tag(IDC_BOOKMARK_THIS_TAB);
}

BookmarkButton::~BookmarkButton() = default;

const char* BookmarkButton::GetClassName() const {
  return "BookmarkButton";
}

void BookmarkButton::SetToggled(bool on) {
  active_ = on;
  UpdateImageAndText();
}

void BookmarkButton::UpdateImageAndText() {
  const ui::ColorProvider* color_provider = GetColorProvider();
  SkColor icon_color = color_provider->GetColor(kColorToolbarButtonIcon);
  const gfx::VectorIcon& icon =
      active_ ? omnibox::kStarActiveIcon : omnibox::kStarIcon;
  SetImage(views::Button::STATE_NORMAL,
           gfx::CreateVectorIcon(icon, 16, icon_color));

  int tooltip_id = active_ ? IDS_TOOLTIP_STARRED : IDS_TOOLTIP_STAR;
  SetTooltipText(brave_l10n::GetLocalizedResourceUTF16String(tooltip_id));
}
