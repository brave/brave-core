/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/views/extensions/brave_extensions_menu_main_page_view.h"

#include <utility>

#include "base/functional/bind.h"
#include "brave/browser/ui/color/brave_color_id.h"
#include "brave/components/vector_icons/vector_icons.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/chrome_pages.h"
#include "chrome/browser/ui/views/extensions/extensions_menu_main_page_view.h"
#include "chrome/grit/generated_resources.h"
#include "components/vector_icons/vector_icons.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/metadata/metadata_impl_macros.h"
#include "ui/base/models/image_model.h"
#include "ui/gfx/geometry/insets.h"
#include "ui/gfx/geometry/size.h"
#include "ui/gfx/image/image.h"
#include "ui/gfx/paint_vector_icon.h"
#include "ui/views/background.h"
#include "ui/views/controls/button/button.h"
#include "ui/views/controls/button/image_button.h"
#include "ui/views/controls/button/image_button_factory.h"
#include "ui/views/controls/highlight_path_generator.h"
#include "ui/views/view_class_properties.h"
#include "ui/views/view_utils.h"

BraveExtensionsMenuMainPageView::BraveExtensionsMenuMainPageView(
    Browser* browser,
    ExtensionsMenuHandler* menu_handler)
    : ExtensionsMenuMainPageView(browser, menu_handler) {
  UpdateButtons(browser);
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

void BraveExtensionsMenuMainPageView::UpdateButtons(Browser* browser) {
  // Swap the order of site setting toggle button and setting button
  auto* parent = views::AsViewClass<views::FlexLayoutView>(
      site_settings_toggle_->parent());
  CHECK(parent) << "Parent has been changed, need to revisit";

  auto index = *parent->GetIndexOf(site_settings_toggle_);
  parent->ReorderChildView(site_settings_toggle_, index - 1);

  // Recreate settings button. Notably, ColorTrackingVectorImageButton is not
  // allowed to change the icon, so we need to recreate the button.
  auto* settings_button =
      views::AsViewClass<views::ImageButton>(parent->children().at(index));
  CHECK(settings_button) << "Settings button has been changed, need to revisit";
  CHECK_EQ(settings_button->GetTooltipText(),
           l10n_util::GetStringUTF16(IDS_MANAGE_EXTENSIONS))
      << "Settings button has been changed, need to revisit";
  parent->RemoveChildViewT(settings_button);

  auto new_setting_button = views::CreateVectorImageButtonWithNativeTheme(
      base::BindRepeating(
          [](Browser* browser) { chrome::ShowExtensions(browser); }, browser),
      kLeoSettingsIcon, /*icon_size*/ 20);
  new_setting_button->SetTooltipText(
      l10n_util::GetStringUTF16(IDS_MANAGE_EXTENSIONS));
  new_setting_button->SetProperty(views::kMarginsKey,
                                  gfx::Insets().set_left(12));
  new_setting_button->SizeToPreferredSize();
  views::InstallCircleHighlightPathGenerator(new_setting_button.get());
  parent->AddChildViewAt(std::move(new_setting_button), index);
}

BEGIN_METADATA(BraveExtensionsMenuMainPageView)
END_METADATA
