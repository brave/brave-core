// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_welcome_page/brave_welcome_page_ui.h"

#include <utility>

#include "brave/browser/resources/brave_welcome_page/grit/brave_welcome_page_generated_map.h"
#include "brave/browser/ui/webui/brave_webui_source.h"
#include "brave/browser/ui/webui/brave_welcome_page/welcome_page_handler.h"
#include "brave/browser/ui/webui/settings/brave_import_bulk_data_handler.h"
#include "brave/components/constants/webui_url_constants.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search/background/ntp_custom_background_service_factory.h"
#include "chrome/browser/themes/theme_service_factory.h"
#include "chrome/browser/ui/webui/cr_components/theme_color_picker/theme_color_picker_handler.h"
#include "chrome/browser/ui/webui/settings/settings_default_browser_handler.h"
#include "components/grit/brave_components_resources.h"
#include "components/grit/brave_components_strings.h"
#include "components/grit/brave_components_webui_strings.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/url_constants.h"
#include "ui/webui/webui_util.h"

namespace {

inline constexpr char kBraveWelcomePageHost[] = "welcome-new";

}  // namespace

BraveWelcomePageUI::BraveWelcomePageUI(content::WebUI* web_ui)
    : ui::MojoWebUIController(web_ui, /*enable_chrome_send=*/true) {
  auto* profile = Profile::FromWebUI(web_ui);
  content::WebUIDataSource* source =
      content::WebUIDataSource::CreateAndAdd(profile, kBraveWelcomePageHost);

  webui::SetupWebUIDataSource(source, kBraveWelcomePageGenerated,
                              IDR_BRAVE_WELCOME_PAGE_HTML);

  AddBackgroundColorToSource(source, web_ui->GetWebContents());

  web_ui->AddMessageHandler(
      std::make_unique<settings::BraveImportBulkDataHandler>());
  web_ui->AddMessageHandler(
      std::make_unique<settings::DefaultBrowserHandler>());

  source->AddLocalizedStrings(webui::kBraveWelcomePageStrings);
  source->AddString("pageTheme", "");
}

BraveWelcomePageUI::~BraveWelcomePageUI() = default;

void BraveWelcomePageUI::BindInterface(
    mojo::PendingReceiver<brave_welcome_page::mojom::WelcomePageHandler>
        receiver) {
  auto* profile = Profile::FromWebUI(web_ui());
  page_handler_ = std::make_unique<brave_welcome_page::WelcomePageHandler>(
      std::move(receiver), ThemeServiceFactory::GetForProfile(profile),
      profile->GetPrefs());
}

void BraveWelcomePageUI::BindInterface(
    mojo::PendingReceiver<
        theme_color_picker::mojom::ThemeColorPickerHandlerFactory> receiver) {
  if (theme_color_picker_handler_factory_receiver_.is_bound()) {
    theme_color_picker_handler_factory_receiver_.reset();
  }
  theme_color_picker_handler_factory_receiver_.Bind(std::move(receiver));
}

void BraveWelcomePageUI::CreateThemeColorPickerHandler(
    mojo::PendingReceiver<theme_color_picker::mojom::ThemeColorPickerHandler>
        handler,
    mojo::PendingRemote<theme_color_picker::mojom::ThemeColorPickerClient>
        client) {
  auto* profile = Profile::FromWebUI(web_ui());
  theme_color_picker_handler_ = std::make_unique<ThemeColorPickerHandler>(
      std::move(handler), std::move(client),
      NtpCustomBackgroundServiceFactory::GetForProfile(profile),
      web_ui()->GetWebContents());
}

WEB_UI_CONTROLLER_TYPE_IMPL(BraveWelcomePageUI)

BraveWelcomePageUIConfig::BraveWelcomePageUIConfig()
    : DefaultWebUIConfig(content::kChromeUIScheme, kBraveWelcomePageHost) {}

bool BraveWelcomePageUIConfig::IsWebUIEnabled(
    content::BrowserContext* browser_context) {
  auto* profile = Profile::FromBrowserContext(browser_context);
  return !profile->IsGuestSession();
}
