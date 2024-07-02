// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_EDUCATION_BRAVE_EDUCATION_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_EDUCATION_BRAVE_EDUCATION_UI_H_

#include <memory>
#include <string>

#include "base/memory/raw_ptr.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "ui/webui/mojo_web_ui_controller.h"
#include "ui/webui/resources/js/browser_command/browser_command.mojom.h"

class BrowserCommandHandler;
class Profile;

namespace content {
class WebUI;
}

namespace brave_education {

// The Web UI controller for the brave://education page.
class BraveEducationUI : public ui::MojoWebUIController,
                         public browser_command::mojom::CommandHandlerFactory {
 public:
  BraveEducationUI(content::WebUI* web_ui, const std::string& host_name);
  ~BraveEducationUI() override;

  BraveEducationUI(const BraveEducationUI&) = delete;
  BraveEducationUI& operator=(const BraveEducationUI&) = delete;

  // Instantiates the an instance of CommandHandlerFactory.
  void BindInterface(
      mojo::PendingReceiver<browser_command::mojom::CommandHandlerFactory>
          pending_receiver);

 private:
  // mojom::CommandHandlerFactory
  void CreateBrowserCommandHandler(
      mojo::PendingReceiver<browser_command::mojom::CommandHandler>
          pending_handler) override;

  mojo::Receiver<browser_command::mojom::CommandHandlerFactory>
      handler_factory_receiver_;
  std::unique_ptr<BrowserCommandHandler> command_handler_;
  raw_ptr<Profile> profile_;

  WEB_UI_CONTROLLER_TYPE_DECL();
};

}  // namespace brave_education

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_EDUCATION_BRAVE_EDUCATION_UI_H_
