// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_BROWSER_UI_WEBUI_COMMANDS_UI_H_
#define BRAVE_BROWSER_UI_WEBUI_COMMANDS_UI_H_

#include <string>

#include "brave/components/commands/common/commands.mojom.h"
#include "brave/components/playlist/common/mojom/playlist.mojom.h"
#include "content/public/browser/web_ui_controller.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/receiver_set.h"

class Browser;
class BraveBrowserWindow;

namespace commands {

class CommandsUI : public content::WebUIController, public CommandsService {
 public:
  CommandsUI(content::WebUI* web_ui, const std::string& host);
  ~CommandsUI() override;
  CommandsUI(const CommandsUI&) = delete;
  CommandsUI& operator=(const CommandsUI&) = delete;

  void BindInterface(mojo::PendingReceiver<CommandsService> pending_receiver);

  // CommandsService:
  void GetCommands(GetCommandsCallback callback) override;
  void TryExecuteCommand(uint32_t command_id) override;

 protected:
  Browser* GetBrowser();
  BraveBrowserWindow* GetWindow();

 private:
  mojo::Receiver<CommandsService> receiver_{this};

  WEB_UI_CONTROLLER_TYPE_DECL();
};

}  // namespace commands

#endif  // BRAVE_BROWSER_UI_WEBUI_COMMANDS_UI_H_
