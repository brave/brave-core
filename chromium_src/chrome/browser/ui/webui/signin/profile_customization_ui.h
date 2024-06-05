/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SIGNIN_PROFILE_CUSTOMIZATION_UI_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SIGNIN_PROFILE_CUSTOMIZATION_UI_H_

#include "ui/webui/resources/cr_components/theme_color_picker/theme_color_picker.mojom.h"

#define CreateThemeColorPickerHandler                                        \
  CreateThemeColorPickerHandler_Unused(                                      \
      mojo::PendingReceiver<                                                 \
          theme_color_picker::mojom::ThemeColorPickerHandler> handler,       \
      mojo::PendingRemote<theme_color_picker::mojom::ThemeColorPickerClient> \
          client);                                                           \
  void CreateThemeColorPickerHandler

#include "src/chrome/browser/ui/webui/signin/profile_customization_ui.h"  // IWYU pragma: export

#undef CreateThemeColorPickerHandler

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SIGNIN_PROFILE_CUSTOMIZATION_UI_H_
