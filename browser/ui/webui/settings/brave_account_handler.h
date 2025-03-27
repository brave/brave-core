/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_ACCOUNT_HANDLER_H_
#define BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_ACCOUNT_HANDLER_H_

#include <string>

#include "brave/components/brave_account/core/common/mojom/brave_account.mojom.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"

namespace brave_account {
class BraveAccountHandler : public mojom::BraveAccountHandler {
 public:
  BraveAccountHandler(mojo::PendingReceiver<mojom::BraveAccountHandler> handler,
                      mojo::PendingRemote<mojom::BraveAccountClient> client);

  BraveAccountHandler(const BraveAccountHandler&) = delete;
  BraveAccountHandler& operator=(const BraveAccountHandler&) = delete;

  ~BraveAccountHandler() override;

  void GetPasswordStrength(
      const std::string& password,
      mojom::BraveAccountHandler::GetPasswordStrengthCallback callback)
      override;

 private:
  mojo::Receiver<mojom::BraveAccountHandler> handler_;
  mojo::Remote<mojom::BraveAccountClient> client_;
};
}  // namespace brave_account

#endif  // BRAVE_BROWSER_UI_WEBUI_SETTINGS_BRAVE_ACCOUNT_HANDLER_H_
