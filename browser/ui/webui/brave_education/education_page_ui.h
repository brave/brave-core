// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_EDUCATION_EDUCATION_PAGE_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_EDUCATION_EDUCATION_PAGE_UI_H_

#include <memory>

#include "brave/components/brave_education/education_page.mojom.h"
#include "brave/components/constants/webui_url_constants.h"
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

namespace brave_education {

// The Web UI controller for the Brave product education page, which displays
// production education website content in an iframe.
class EducationPageUI : public ui::MojoWebUIController,
                        public mojom::EducationPageHandlerFactory {
 public:
  EducationPageUI(content::WebUI* web_ui, const GURL& url);
  ~EducationPageUI() override;

  EducationPageUI(const EducationPageUI&) = delete;
  EducationPageUI& operator=(const EducationPageUI&) = delete;

  void BindInterface(
      mojo::PendingReceiver<EducationPageHandlerFactory> pending_receiver);

  // mojom::EducationPageHandlerFactory:
  void CreatePageHandler(
      mojo::PendingReceiver<mojom::EducationPageHandler> handler) override;

 private:
  mojo::Receiver<EducationPageHandlerFactory> page_factory_receiver_{this};
  std::unique_ptr<mojom::EducationPageHandler> page_handler_;

  WEB_UI_CONTROLLER_TYPE_DECL();
};

class GettingStartedUIConfig
    : public content::DefaultWebUIConfig<EducationPageUI> {
 public:
  GettingStartedUIConfig()
      : DefaultWebUIConfig(content::kChromeUIScheme, kBraveGettingStartedHost) {
  }
};

}  // namespace brave_education

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_EDUCATION_EDUCATION_PAGE_UI_H_
