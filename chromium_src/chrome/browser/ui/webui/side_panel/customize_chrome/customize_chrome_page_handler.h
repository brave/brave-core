// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SIDE_PANEL_CUSTOMIZE_CHROME_CUSTOMIZE_CHROME_PAGE_HANDLER_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SIDE_PANEL_CUSTOMIZE_CHROME_CUSTOMIZE_CHROME_PAGE_HANDLER_H_

#include "chrome/browser/ui/webui/side_panel/customize_chrome/customize_chrome.mojom.h"

// Declares our own version of CustomizeChromePageHandler before replacing
// Chromium's class name in order to avoid name conflicts.
class CustomizeChromePageHandler;
using BraveCustomizeChromePageHandler = CustomizeChromePageHandler;

namespace side_panel::mojom {
using CustomizeChromePageHandler_Chromium = CustomizeChromePageHandler;
}  // namespace side_panel::mojom

// In order to replace the Chromium version of CustomizeChromePageHandler with
// our own version, changes Chromium version's name to
// CustomizeChromePageHandler_Chromium
#define CustomizeChromePageHandler CustomizeChromePageHandler_Chromium

// Add friend declaration
#define ScrollToSection                   \
  ScrollToSection_Unused() {}             \
  friend BraveCustomizeChromePageHandler; \
  void ScrollToSection

#include <chrome/browser/ui/webui/side_panel/customize_chrome/customize_chrome_page_handler.h>  // IWYU pragma: export

#undef ScrollToSection
#undef CustomizeChromePageHandler

class CustomizeChromeUI;

// We're extending the CustomizeChromePageHandler to:
// * Add methods to close the side panel from the web UI.
// * Add methods to (un)set pref for "Darker Theme"
class CustomizeChromePageHandler : public CustomizeChromePageHandler_Chromium {
 public:
  CustomizeChromePageHandler(
      mojo::PendingReceiver<side_panel::mojom::CustomizeChromePageHandler>
          pending_page_handler,
      mojo::PendingRemote<side_panel::mojom::CustomizeChromePage> pending_page,
      NtpCustomBackgroundService* ntp_custom_background_service,
      content::WebContents* web_contents,
      const std::vector<ntp::ModuleIdDetail> module_id_details,
      std::optional<base::RepeatingCallback<void(const GURL&)>>
          open_url_callback = std::nullopt);
  ~CustomizeChromePageHandler() override;

  void set_customize_chrome_ui(
      const base::WeakPtr<CustomizeChromeUI>& customize_chrome_ui) {
    customize_chrome_ui_ = std::move(customize_chrome_ui);
  }

  // CustomizeChromePageHandler_Chromium:
  void ClosePanel() override;
  void GetUseDarkerTheme(GetUseDarkerThemeCallback callback) override;
  void SetUseDarkerTheme(bool use_darker_theme) override;

 private:
#if defined(TOOLKIT_VIEWS)
  // Notifies when the pref for "Darker Theme" changes.
  void NotifyUseDarkerThemeChanged();
#endif  // defined(TOOLKIT_VIEWS)

  // Needed to close the side panel.
  base::WeakPtr<CustomizeChromeUI> customize_chrome_ui_;
};

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_WEBUI_SIDE_PANEL_CUSTOMIZE_CHROME_CUSTOMIZE_CHROME_PAGE_HANDLER_H_
