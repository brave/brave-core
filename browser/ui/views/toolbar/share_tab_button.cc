/* Copyright 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/toolbar/share_tab_button.h"

#include <utility>

#include "base/strings/utf_string_conversions.h"
#include "brave/app/vector_icons/vector_icons.h"
#include "brave/components/l10n/common/locale_util.h"
#include "chrome/app/chrome_command_ids.h"
#include "chrome/browser/themes/theme_properties.h"
#include "chrome/browser/ui/view_ids.h"
#include "chrome/browser/ui/views/toolbar/toolbar_view.h"
#include "chrome/grit/generated_resources.h"
#include "components/omnibox/browser/vector_icons.h"
#include "components/strings/grit/components_strings.h"
#include "components/vector_icons/vector_icons.h"
#include "ui/base/theme_provider.h"
#include "ui/gfx/paint_vector_icon.h"
#include "brave/browser/ui/brave_view_ids.h"
#include "ui/base/metadata/metadata_impl_macros.h"

namespace share_tab_button {

ShareTabButton::~ShareTabButton() = default;
ShareTabButton::ShareTabButton(PressedCallback callback)
    : ToolbarButton(callback) {
  SetID(BRAVE_VIEW_ID_SHARE_TAB_BUTTON);
}

void ShareTabButton::UpdateImageAndText() {
  LOG(ERROR) << "Updated sharetabbutton: " << (HasImage(views::Button::STATE_NORMAL));
  const ui::ThemeProvider* tp = GetThemeProvider();

  SkColor icon_color = tp->GetColor(ThemeProperties::COLOR_TOOLBAR_BUTTON_ICON);
  const gfx::VectorIcon& icon = kWalletToolbarButtonIcon;
  auto image = gfx::CreateVectorIcon(icon, 48, icon_color);
  SetImage(views::Button::STATE_NORMAL, image);

  int tooltip_id = IDS_ACCESS_CODE_CAST_CONNECT;
  SetTooltipText(brave_l10n::GetLocalizedResourceUTF16String(tooltip_id));
}

BEGIN_METADATA(ShareTabButton, ToolbarButton)
END_METADATA

}  // namespace share_tab_button

