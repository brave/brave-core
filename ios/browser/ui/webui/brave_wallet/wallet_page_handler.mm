// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/ios/browser/ui/webui/brave_wallet/wallet_page_handler.h"

#include <utility>

WalletPageHandler::WalletPageHandler(
    mojo::PendingReceiver<brave_wallet::mojom::PageHandler> receiver)
    : receiver_(this, std::move(receiver)) {}

WalletPageHandler::~WalletPageHandler() = default;

void WalletPageHandler::ShowApprovePanelUI() {}
void WalletPageHandler::ShowWalletBackupUI() {}
