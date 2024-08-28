/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/extensions/brave_extensions_menu_main_page_view.h"

#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/views/controls/hover_button.h"
#include "chrome/browser/ui/views/extensions/extensions_menu_main_page_view.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/models/image_model.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/view_utils.h"

BraveExtensionsMenuMainPageView::BraveExtensionsMenuMainPageView(
    Browser* browser,
    ExtensionsMenuHandler* menu_handler)
    : ExtensionsMenuMainPageView(browser, menu_handler) {
  UpdateButtons();
}

BraveExtensionsMenuMainPageView::~BraveExtensionsMenuMainPageView() = default;

void BraveExtensionsMenuMainPageView::OnThemeChanged() {
  ExtensionsMenuMainPageView::OnThemeChanged();

  auto* cp = GetColorProvider();
  CHECK(cp);

  site_settings_toggle_->SetThumbOnColor(SK_ColorWHITE);
  site_settings_toggle_->SetTrackOnColor(
      cp->GetColor(kColorBraveExtensionMenuIcon));
}

void BraveExtensionsMenuMainPageView::UpdateButtons() {
  // Settings is the last child of the menu.
  size_t settings_button_index = children().size() - 1;
  auto* settings_button =
      views::AsViewClass<HoverButton>(children().at(settings_button_index));
  CHECK(settings_button) << "Settings button has been changed, need to revisit";
  CHECK_EQ(settings_button->GetText(),
           l10n_util::GetStringUTF16(IDS_MANAGE_EXTENSIONS))
      << "Settings button has been changed, need to revisit";
  settings_button->SetImageModel(
      views::Button::STATE_NORMAL,
      ui::ImageModel::FromVectorIcon(kLeoSettingsIcon));
}

BEGIN_METADATA(BraveExtensionsMenuMainPageView)
END_METADATA
