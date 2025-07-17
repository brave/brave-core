/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_ACCOUNT_SETTINGS_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_ACCOUNT_SETTINGS_HANDLER_H_

#include "base/memory/raw_ptr.h"
#include "brave/components/brave_account/mojom/brave_account_settings_handler.mojom.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"

namespace content {
class WebUI;
}  // namespace content

namespace brave_account {
class BraveAccountSettingsHandler : public mojom::BraveAccountSettingsHandler {
 public:
  explicit BraveAccountSettingsHandler(
      mojo::PendingReceiver<mojom::BraveAccountSettingsHandler> handler,
      content::WebUI* web_ui);

  BraveAccountSettingsHandler(const BraveAccountSettingsHandler&) = delete;
  BraveAccountSettingsHandler& operator=(const BraveAccountSettingsHandler&) =
      delete;

  ~BraveAccountSettingsHandler() override;

  void OpenDialog() override;

 private:
  mojo::Receiver<mojom::BraveAccountSettingsHandler> handler_;
  raw_ptr<content::WebUI> web_ui_;
};
}  // namespace brave_account

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_ACCOUNT_SETTINGS_HANDLER_H_
