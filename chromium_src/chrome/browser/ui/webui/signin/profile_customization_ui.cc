/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "chrome/browser/ui/webui/signin/profile_customization_ui.h"

#include "brave/browser/ui/webui/cr_components/theme_color_picker/brave_theme_color_picker_handler.h"

#define CreateThemeColorPickerHandler CreateThemeColorPickerHandler_Unused
#include "src/chrome/browser/ui/webui/signin/profile_customization_ui.cc"
#undef CreateThemeColorPickerHandler

void ProfileCustomizationUI::CreateThemeColorPickerHandler(
    mojo::PendingReceiver<theme_color_picker::mojom::ThemeColorPickerHandler>
        handler,
    mojo::PendingRemote<theme_color_picker::mojom::ThemeColorPickerClient>
        client) {
  theme_color_picker_handler_ = std::make_unique<BraveThemeColorPickerHandler>(
      std::move(handler), std::move(client),
      NtpCustomBackgroundServiceFactory::GetForProfile(
          Profile::FromWebUI(web_ui())),
      web_ui()->GetWebContents());
}
