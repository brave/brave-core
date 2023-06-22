// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_wallet/android/android_wallet_page_handler.h"

#include <utility>

#include "brave/browser/brave_wallet/brave_wallet_provider_delegate_impl_helper.h"

template <typename T>
AndroidWalletPageHandler<T>::AndroidWalletPageHandler(
    mojo::PendingReceiver<brave_wallet::mojom::PageHandler> receiver,
    Profile* profile,
    ui::MojoWebUIController* webui_controller)
    : WalletPageHandler(std::move(receiver), profile),
      webui_controller_(webui_controller) {
  DCHECK(webui_controller_);
}
template <typename T>
AndroidWalletPageHandler<T>::~AndroidWalletPageHandler() = default;

template <typename T>
void AndroidWalletPageHandler<T>::ShowApprovePanelUI() {
  if (!webui_controller_) {
    return;
  }

  ::brave_wallet::ShowPanel(webui_controller_->web_ui()->GetWebContents());
}

SendPageHandler::SendPageHandler(
    mojo::PendingReceiver<brave_wallet::mojom::PageHandler> receiver,
    Profile* profile,
    ui::MojoWebUIController* webui_controller)
    : AndroidWalletPageHandler<SendPageHandler>(std::move(receiver),
                                                profile,
                                                webui_controller) {}

SwapPageHandler::SwapPageHandler(
    mojo::PendingReceiver<brave_wallet::mojom::PageHandler> receiver,
    Profile* profile,
    ui::MojoWebUIController* webui_controller)
    : AndroidWalletPageHandler<SwapPageHandler>(std::move(receiver),
                                                profile,
                                                webui_controller) {}
