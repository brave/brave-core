/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_BRAVE_BROWSER_COMMAND_BRAVE_BROWSER_COMMAND_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_BRAVE_BROWSER_COMMAND_BRAVE_BROWSER_COMMAND_HANDLER_H_

#include <memory>
#include <string_view>
#include <utility>

#include "brave/components/brave_education/education_urls.h"
#include "brave/ui/webui/resources/js/brave_browser_command/brave_browser_command.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "ui/base/window_open_disposition.h"

class Profile;

class BraveBrowserCommandHandler
    : public brave_browser_command::mojom::BraveBrowserCommandHandler {
 public:
  // Handles platform-specific browser-level education tasks.
  class Delegate {
   public:
    virtual ~Delegate() = default;
    virtual void OpenURL(const GURL& url,
                         WindowOpenDisposition disposition) = 0;
    virtual void OpenRewardsPanel() = 0;
    virtual void OpenVPNPanel() = 0;
    virtual void OpenAIChat() = 0;
  };

  BraveBrowserCommandHandler(
      mojo::PendingReceiver<
          brave_browser_command::mojom::BraveBrowserCommandHandler> receiver,
      Profile* profile,
      brave_education::EducationPageType page_type,
      std::unique_ptr<Delegate> delegate);

  ~BraveBrowserCommandHandler() override;

  static constexpr std::string_view kChildSrcDirective =
      "child-src chrome://webui-test https://brave.com/;";

  // brave_browser_command::mojom::BraveBrowserCommandHandler:
  void GetServerUrl(GetServerUrlCallback callback) override;

  void ExecuteCommand(brave_browser_command::mojom::Command command,
                      ExecuteCommandCallback callback) override;

 private:
  bool CanExecute(brave_browser_command::mojom::Command command);

  mojo::Receiver<brave_browser_command::mojom::BraveBrowserCommandHandler>
      receiver_;
  raw_ptr<Profile> profile_;
  brave_education::EducationPageType page_type_;
  std::unique_ptr<Delegate> delegate_;
};

#endif  // BRAVE_BROWSER_UI_WEBUI_BRAVE_BROWSER_COMMAND_BRAVE_BROWSER_COMMAND_HANDLER_H_
