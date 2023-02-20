// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_COMMANDS_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_COMMANDS_UI_H_

#include <string>

#include "brave/components/commands/common/commands.mojom.h"
#include "content/public/browser/web_ui_controller.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/pending_remote.h"

namespace commands {

class CommandsUI : public content::WebUIController {
 public:
  CommandsUI(content::WebUI* web_ui, const std::string& host);
  ~CommandsUI() override;
  CommandsUI(const CommandsUI&) = delete;
  CommandsUI& operator=(const CommandsUI&) = delete;

  void BindInterface(
      mojo::PendingReceiver<mojom::CommandsService> pending_receiver);

 protected:
  WEB_UI_CONTROLLER_TYPE_DECL();
};

}  // namespace commands

#endif  // BRAVE_BROWSER_UI_WEBUI_COMMANDS_UI_H_
