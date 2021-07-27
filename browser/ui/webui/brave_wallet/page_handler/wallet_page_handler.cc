// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_wallet/page_handler/wallet_page_handler.h"

#include <utility>

#include "brave/components/brave_wallet/browser/hd_keyring.h"
#include "chrome/browser/profiles/profile.h"

WalletPageHandler::WalletPageHandler(
    mojo::PendingReceiver<brave_wallet::mojom::PageHandler> receiver,
    Profile* profile)
    : receiver_(this, std::move(receiver)),
      weak_ptr_factory_(this) {}

WalletPageHandler::~WalletPageHandler() = default;
