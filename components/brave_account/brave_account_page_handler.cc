/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/brave_account_page_handler.h"

#include <string>
#include <utility>

#include "brave/components/brave_account/brave_account_service.h"

BraveAccountPageHandler::BraveAccountPageHandler(
    brave_account::BraveAccountService* brave_account_service,
    mojo::PendingReceiver<brave_account::mojom::PageHandler> receiver)
    : brave_account_service_(brave_account_service),
      receiver_(this, std::move(receiver)) {}

BraveAccountPageHandler::~BraveAccountPageHandler() = default;

void BraveAccountPageHandler::RegisterInitialize(
    const std::string& email,
    const std::string& blinded_message,
    RegisterInitializeCallback callback) {
  brave_account_service_->RegisterInitialize(
      email, blinded_message,
      base::BindOnce(&BraveAccountPageHandler::OnRegisterInitialize,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveAccountPageHandler::OnRegisterInitialize(
    RegisterInitializeCallback callback,
    const std::string& serialized_response) {
  std::move(callback).Run(serialized_response);
}

void BraveAccountPageHandler::RegisterFinalize(
    const std::string& serialized_record,
    RegisterFinalizeCallback callback) {
  brave_account_service_->RegisterFinalize(
      serialized_record,
      base::BindOnce(&BraveAccountPageHandler::OnRegisterFinalize,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveAccountPageHandler::OnRegisterFinalize(
    RegisterFinalizeCallback callback,
    bool success) {
  std::move(callback).Run(success);
}
