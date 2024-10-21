/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BROWSER_COMMAND_BRAVE_BROWSER_COMMAND_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_BROWSER_COMMAND_BRAVE_BROWSER_COMMAND_HANDLER_H_

#include <memory>
#include <utility>
#include <vector>

#include "base/memory/raw_ptr.h"
#include "chrome/browser/ui/webui/browser_command/browser_command_handler.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "ui/base/window_open_disposition.h"
#include "url/gurl.h"

class Profile;

// A handler for commands that are dispatched from web content (typically
// embedded in an iframe). `BraveBrowserCommandHandler` adds support for
// Brave-specific commands.
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

  class BrowserDelegate {
   public:
    virtual ~BrowserDelegate() = default;

    virtual void OpenURL(const GURL& url,
                         WindowOpenDisposition disposition) = 0;
    virtual void OpenRewardsPanel() = 0;
    virtual void OpenVPNPanel() = 0;
    virtual void ExecuteBrowserCommand(int command_id) = 0;
  };

  void SetBrowserDelegateForTesting(std::unique_ptr<BrowserDelegate> delegate) {
    delegate_ = std::move(delegate);
  }

 private:
  bool CanExecute(browser_command::mojom::Command command);

  std::unique_ptr<BrowserDelegate> delegate_;
  raw_ptr<Profile> profile_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BROWSER_COMMAND_BRAVE_BROWSER_COMMAND_HANDLER_H_
