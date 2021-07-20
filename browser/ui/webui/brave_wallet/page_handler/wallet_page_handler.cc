// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at http://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/brave_wallet/page_handler/wallet_page_handler.h"

#include <utility>

#include "brave/browser/brave_wallet/keyring_controller_factory.h"
#include "brave/components/brave_wallet/browser/hd_keyring.h"
#include "brave/components/brave_wallet/browser/keyring_controller.h"
#include "chrome/browser/profiles/profile.h"

WalletPageHandler::WalletPageHandler(
    mojo::PendingReceiver<brave_wallet::mojom::PageHandler> receiver,
    Profile* profile)
    : receiver_(this, std::move(receiver)),
      profile_(profile),
      weak_ptr_factory_(this) {}

WalletPageHandler::~WalletPageHandler() = default;

void WalletPageHandler::EnsureConnected() {
  if (!keyring_controller_) {
    auto pending =
        brave_wallet::KeyringControllerFactory::GetInstance()->GetForContext(
            profile_);
    keyring_controller_.Bind(std::move(pending));
  }
  DCHECK(keyring_controller_);
  keyring_controller_.set_disconnect_handler(base::BindOnce(
      &WalletPageHandler::OnConnectionError, weak_ptr_factory_.GetWeakPtr()));
}

void WalletPageHandler::OnConnectionError() {
  keyring_controller_.reset();
  EnsureConnected();
}

void WalletPageHandler::CreateWallet(const std::string& password,
                                     CreateWalletCallback callback) {
  EnsureConnected();
  keyring_controller_->CreateWallet(password, std::move(callback));
}

void WalletPageHandler::AddAccountToWallet(
    AddAccountToWalletCallback callback) {
  EnsureConnected();
  keyring_controller_->AddAccount(std::move(callback));
}

void WalletPageHandler::GetRecoveryWords(GetRecoveryWordsCallback callback) {
  EnsureConnected();
  keyring_controller_->GetMnemonicForDefaultKeyring(std::move(callback));
}

void WalletPageHandler::RestoreWallet(const std::string& mnemonic,
                                      const std::string& password,
                                      RestoreWalletCallback callback) {
  EnsureConnected();
  keyring_controller_->RestoreWallet(mnemonic, password, std::move(callback));
}
