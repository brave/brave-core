/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_CR_COMPONENTS_THEME_COLOR_PICKER_BRAVE_THEME_COLOR_PICKER_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_CR_COMPONENTS_THEME_COLOR_PICKER_BRAVE_THEME_COLOR_PICKER_HANDLER_H_

#include "chrome/browser/ui/webui/cr_components/theme_color_picker/theme_color_picker_handler.h"

namespace content {
class WebContents;
}  // namespace content

class BraveThemeColorPickerHandler : public ThemeColorPickerHandler {
 public:
  BraveThemeColorPickerHandler(
      mojo::PendingReceiver<theme_color_picker::mojom::ThemeColorPickerHandler>
          pending_handler,
      mojo::PendingRemote<theme_color_picker::mojom::ThemeColorPickerClient>
          pending_client,
      NtpCustomBackgroundService* ntp_custom_background_service,
      content::WebContents* web_contents);

  BraveThemeColorPickerHandler(const BraveThemeColorPickerHandler&) = delete;
  BraveThemeColorPickerHandler& operator=(const BraveThemeColorPickerHandler&) =
      delete;

  ~BraveThemeColorPickerHandler() override = default;

  // side_panel::mojom::CustomizeChromePageHandler:
  void SetSeedColor(SkColor seed_color,
                    ui::mojom::BrowserColorVariant variant) override;
  void GetChromeColors(bool is_dark_mode,
                       bool extended_list,
                       GetChromeColorsCallback callback) override;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_CR_COMPONENTS_THEME_COLOR_PICKER_BRAVE_THEME_COLOR_PICKER_HANDLER_H_
