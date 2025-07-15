// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_CR_COMPONENTS_CUSTOMIZE_COLOR_SCHEME_MODE_BRAVE_CUSTOMIZE_COLOR_SCHEME_MODE_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_CR_COMPONENTS_CUSTOMIZE_COLOR_SCHEME_MODE_BRAVE_CUSTOMIZE_COLOR_SCHEME_MODE_HANDLER_H_

#include "chrome/browser/ui/webui/cr_components/customize_color_scheme_mode/customize_color_scheme_mode_handler.h"
#include "components/prefs/pref_change_registrar.h"

// This class overrides the default behavior of CustomizeColorSchemeModeHandler
// to use Brave's dark mode utilities instead of the ThemeService for color
// scheme mode handling. It listens for changes in the local state preference
// `kBraveDarkMode` and updates the color scheme mode accordingly.
class BraveCustomizeColorSchemeModeHandler
    : public CustomizeColorSchemeModeHandler {
 public:
  explicit BraveCustomizeColorSchemeModeHandler(
      mojo::PendingRemote<
          customize_color_scheme_mode::mojom::CustomizeColorSchemeModeClient>
          pending_client,
      mojo::PendingReceiver<
          customize_color_scheme_mode::mojom::CustomizeColorSchemeModeHandler>
          pending_receiver,
      Profile* profile);

  // CustomizeColorSchemeModeHandler:
  void SetColorSchemeMode(
      customize_color_scheme_mode::mojom::ColorSchemeMode color) override;
  void InitializeColorSchemeMode() override;
  void OnThemeChanged() override;
  void UpdateColorSchemeMode() override;

 private:
  PrefChangeRegistrar local_state_change_registrar_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_CR_COMPONENTS_CUSTOMIZE_COLOR_SCHEME_MODE_BRAVE_CUSTOMIZE_COLOR_SCHEME_MODE_HANDLER_H_
