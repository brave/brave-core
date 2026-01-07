// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_WELCOME_PAGE_BRAVE_WELCOME_PAGE_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_WELCOME_PAGE_BRAVE_WELCOME_PAGE_UI_H_

#include <memory>

#include "brave/browser/ui/webui/brave_welcome_page/brave_welcome_page.mojom.h"
#include "content/public/browser/webui_config.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "ui/webui/mojo_web_ui_controller.h"
#include "ui/webui/resources/cr_components/theme_color_picker/theme_color_picker.mojom.h"

class ThemeColorPickerHandler;

namespace brave_welcome_page {
class WelcomePageHandler;
}  // namespace brave_welcome_page

// The Web UI controller for the Brave welcome page.
class BraveWelcomePageUI
    : public ui::MojoWebUIController,
      public theme_color_picker::mojom::ThemeColorPickerHandlerFactory {
 public:
  explicit BraveWelcomePageUI(content::WebUI* web_ui);
  ~BraveWelcomePageUI() override;

  BraveWelcomePageUI(const BraveWelcomePageUI&) = delete;
  BraveWelcomePageUI& operator=(const BraveWelcomePageUI&) = delete;

  void BindInterface(
      mojo::PendingReceiver<brave_welcome_page::mojom::WelcomePageHandler>
          receiver);

  void BindInterface(
      mojo::PendingReceiver<
          theme_color_picker::mojom::ThemeColorPickerHandlerFactory> receiver);

 private:
  // theme_color_picker::mojom::ThemeColorPickerHandlerFactory:
  void CreateThemeColorPickerHandler(
      mojo::PendingReceiver<theme_color_picker::mojom::ThemeColorPickerHandler>
          handler,
      mojo::PendingRemote<theme_color_picker::mojom::ThemeColorPickerClient>
          client) override;

  std::unique_ptr<brave_welcome_page::WelcomePageHandler> page_handler_;

  std::unique_ptr<ThemeColorPickerHandler> theme_color_picker_handler_;
  mojo::Receiver<theme_color_picker::mojom::ThemeColorPickerHandlerFactory>
      theme_color_picker_handler_factory_receiver_{this};

  WEB_UI_CONTROLLER_TYPE_DECL();
};

class BraveWelcomePageUIConfig
    : public content::DefaultWebUIConfig<BraveWelcomePageUI> {
 public:
  BraveWelcomePageUIConfig();

  // WebUIConfig:
  bool IsWebUIEnabled(content::BrowserContext* browser_context) override;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_WELCOME_PAGE_BRAVE_WELCOME_PAGE_UI_H_
