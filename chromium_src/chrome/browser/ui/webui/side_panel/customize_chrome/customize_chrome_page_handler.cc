// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "chrome/browser/ui/webui/side_panel/customize_chrome/customize_chrome_page_handler.h"

#include "brave/browser/ui/color/pref_names.h"
#include "chrome/browser/ui/webui/side_panel/customize_chrome/customize_chrome_ui.h"

#define CustomizeChromePageHandler CustomizeChromePageHandler_Chromium

#include <chrome/browser/ui/webui/side_panel/customize_chrome/customize_chrome_page_handler.cc>

#undef CustomizeChromePageHandler

CustomizeChromePageHandler::CustomizeChromePageHandler(
    mojo::PendingReceiver<side_panel::mojom::CustomizeChromePageHandler>
        pending_page_handler,
    mojo::PendingRemote<side_panel::mojom::CustomizeChromePage> pending_page,
    NtpCustomBackgroundService* ntp_custom_background_service,
    content::WebContents* web_contents,
    const std::vector<ntp::ModuleIdDetail> module_id_details,
    std::optional<base::RepeatingCallback<void(const GURL&)>> open_url_callback)
    : CustomizeChromePageHandler_Chromium(std::move(pending_page_handler),
                                          std::move(pending_page),
                                          ntp_custom_background_service,
                                          web_contents,
                                          module_id_details,
                                          open_url_callback) {
  pref_change_registrar_.Add(
      color::prefs::kBraveDarkerMode,
      base::BindRepeating(
          &CustomizeChromePageHandler::NotifyUseDarkerThemeChanged,
          base::Unretained(this)));
}

CustomizeChromePageHandler::~CustomizeChromePageHandler() = default;

void CustomizeChromePageHandler::ClosePanel() {
  CHECK(customize_chrome_ui_)
      << "CustomizeChromeUI must be set on its creation.";
  auto close_panel = customize_chrome_ui_->close_panel_callback();
  CHECK(close_panel);
  close_panel.Run();
}

void CustomizeChromePageHandler::GetUseDarkerTheme(
    GetUseDarkerThemeCallback callback) {
  const bool use_darker_theme =
      profile_->GetPrefs()->GetBoolean(color::prefs::kBraveDarkerMode);
  std::move(callback).Run(use_darker_theme);
}

void CustomizeChromePageHandler::SetUseDarkerTheme(bool use_darker_theme) {
  profile_->GetPrefs()->SetBoolean(color::prefs::kBraveDarkerMode,
                                   use_darker_theme);
}

void CustomizeChromePageHandler::NotifyUseDarkerThemeChanged() {
  const bool use_darker_theme =
      profile_->GetPrefs()->GetBoolean(color::prefs::kBraveDarkerMode);
  page_->OnUseDarkerThemeChanged(use_darker_theme);
}
