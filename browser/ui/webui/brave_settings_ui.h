/* Copyright 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_SETTINGS_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_SETTINGS_UI_H_

#include <memory>
#include <string>

#include "chrome/browser/ui/webui/settings/settings_ui.h"

#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
#include "brave/components/commands/common/commands.mojom.h"
#endif

namespace content {
class WebUIDataSource;
}

class Profile;

class BraveSettingsUI : public settings::SettingsUI {
 public:
  BraveSettingsUI(content::WebUI* web_ui, const std::string& host);
  BraveSettingsUI(const BraveSettingsUI&) = delete;
  BraveSettingsUI& operator=(const BraveSettingsUI&) = delete;
  ~BraveSettingsUI() override;

  static void AddResources(content::WebUIDataSource* html_source,
                           Profile* profile);
  // Allows disabling CSP on settings page so EvalJS could be run in main world.
  static bool& ShouldDisableCSPForTesting();

  static bool& ShouldExposeElementsForTesting();

#if !BUILDFLAG(IS_ANDROID) && !BUILDFLAG(IS_IOS)
  void BindInterface(
      mojo::PendingReceiver<commands::mojom::CommandsService> pending_receiver);
#endif
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_SETTINGS_UI_H_
