/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BROWSER_COMMAND_BRAVE_BROWSER_COMMAND_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_BROWSER_COMMAND_BRAVE_BROWSER_COMMAND_HANDLER_H_

#include <vector>

#include "base/memory/raw_ptr.h"
#include "chrome/browser/ui/webui/browser_command/browser_command_handler.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "ui/base/window_open_disposition.h"

class Profile;

class BraveBrowserCommandHandler : public BrowserCommandHandler {
 public:
  BraveBrowserCommandHandler(
      mojo::PendingReceiver<browser_command::mojom::CommandHandler>
          pending_command_handler,
      Profile* profile,
      std::vector<browser_command::mojom::Command> supported_commands);

  ~BraveBrowserCommandHandler() override;

  // mojom::CommandHandler:
  void CanExecuteCommand(browser_command::mojom::Command command,
                         CanExecuteCommandCallback callback) override;

  // CommandUpdaterDelegate:
  void ExecuteCommandWithDisposition(
      int command_id,
      WindowOpenDisposition disposition) override;

 private:
  std::optional<bool> CanExecute(browser_command::mojom::Command command);

  raw_ptr<Profile> profile_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BROWSER_COMMAND_BRAVE_BROWSER_COMMAND_HANDLER_H_
