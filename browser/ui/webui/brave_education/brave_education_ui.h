// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_EDUCATION_BRAVE_EDUCATION_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_EDUCATION_BRAVE_EDUCATION_UI_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_education/common/education_page.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "ui/webui/mojo_web_ui_controller.h"
#include "ui/webui/resources/js/browser_command/browser_command.mojom.h"

namespace content {
class WebUI;
}

namespace brave_education {

// The Web UI controller for the Brave product education page, which displays
// production education website content in an iframe.
class BraveEducationUI : public ui::MojoWebUIController,
                         public mojom::EducationPageHandlerFactory,
                         public browser_command::mojom::CommandHandlerFactory {
 public:
  BraveEducationUI(content::WebUI* web_ui, const std::string& host_name);
  ~BraveEducationUI() override;

  BraveEducationUI(const BraveEducationUI&) = delete;
  BraveEducationUI& operator=(const BraveEducationUI&) = delete;

  void BindInterface(
      mojo::PendingReceiver<EducationPageHandlerFactory> pending_receiver);

  void BindInterface(
      mojo::PendingReceiver<CommandHandlerFactory> pending_receiver);

  // mojom::EducationPageHandlerFactory:
  void CreatePageHandler(
      mojo::PendingReceiver<mojom::EducationPageHandler> handler) override;

  // browser_command::mojom::CommandHandlerFactory:
  void CreateBrowserCommandHandler(
      mojo::PendingReceiver<browser_command::mojom::CommandHandler> handler)
      override;

 private:
  mojo::Receiver<EducationPageHandlerFactory> page_factory_receiver_{this};
  mojo::Receiver<CommandHandlerFactory> command_handler_factory_receiver_{this};
  std::unique_ptr<mojom::EducationPageHandler> page_handler_;
  std::unique_ptr<browser_command::mojom::CommandHandler> command_handler_;

  WEB_UI_CONTROLLER_TYPE_DECL();
};

}  // namespace brave_education

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_EDUCATION_BRAVE_EDUCATION_UI_H_
