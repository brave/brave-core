// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

// based on //chrome/browser/ui/webui/whats_new/whats_new_ui.h

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_EDUCATION_EDUCATION_PAGE_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_EDUCATION_EDUCATION_PAGE_UI_H_

#include <memory>

#include "brave/browser/ui/webui/brave_browser_command/brave_browser_command_handler.h"
#include "brave/browser/ui/webui/brave_education/brave_education.mojom.h"
#include "brave/components/constants/webui_url_constants.h"
#include "brave/ui/webui/resources/js/brave_browser_command/brave_browser_command.mojom.h"
#include "content/public/browser/webui_config.h"
#include "content/public/common/url_constants.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "ui/webui/mojo_web_ui_controller.h"
#include "url/gurl.h"

namespace content {
class WebUI;
}

class BraveEducationHandler;
class EducationPageUI;

class EducationPageUIConfig
    : public content::DefaultWebUIConfig<EducationPageUI> {
 public:
  EducationPageUIConfig()
      : DefaultWebUIConfig(content::kChromeUIScheme, kBraveGettingStartedHost) {
  }
};

// The Web UI controller for the Brave product education page, which displays
// production education website content in an iframe.
class EducationPageUI
    : public ui::MojoWebUIController,
      public brave_browser_command::mojom::BraveBrowserCommandHandlerFactory,
      public brave_education::mojom::PageHandlerFactory {
 public:
  EducationPageUI(content::WebUI* web_ui, const GURL& url);
  ~EducationPageUI() override;

  // Instantiates the implementor of the
  // brave_education::mojom::PageHandlerFactory mojo interface.
  void BindInterface(
      mojo::PendingReceiver<brave_education::mojom::PageHandlerFactory>
          receiver);

  // Instantiates the implementor of the
  // browser_command::mojom::BraveBrowserCommandHandler mojo interface.
  void BindInterface(
      mojo::PendingReceiver<
          brave_browser_command::mojom::BraveBrowserCommandHandlerFactory>
          pending_receiver);

  EducationPageUI(const EducationPageUI&) = delete;
  EducationPageUI& operator=(const EducationPageUI&) = delete;

 private:
  // brave_education::mojom::PageHandlerFactory:
  void CreatePageHandler(
      mojo::PendingRemote<brave_education::mojom::Page> page,
      mojo::PendingReceiver<brave_education::mojom::PageHandler> receiver)
      override;

  std::unique_ptr<BraveEducationHandler> page_handler_;
  mojo::Receiver<brave_education::mojom::PageHandlerFactory>
      page_factory_receiver_{this};

  // brave_browser_command::mojom::BraveBrowserCommandHandlerFactory:
  void CreateBrowserCommandHandler(
      mojo::PendingReceiver<
          brave_browser_command::mojom::BraveBrowserCommandHandler> handler)
      override;

  std::unique_ptr<BraveBrowserCommandHandler> command_handler_;
  mojo::Receiver<
      brave_browser_command::mojom::BraveBrowserCommandHandlerFactory>
      browser_command_factory_receiver_;
  raw_ptr<Profile> profile_;

  WEB_UI_CONTROLLER_TYPE_DECL();
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_EDUCATION_EDUCATION_PAGE_UI_H_
