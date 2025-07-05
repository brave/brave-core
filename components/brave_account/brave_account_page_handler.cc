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
    mojo::PendingReceiver<brave_account::mojom::PageHandler> pending_receiver)
    : brave_account_service_(brave_account_service),
      receiver_(this, std::move(pending_receiver)) {}

BraveAccountPageHandler::~BraveAccountPageHandler() = default;

void BraveAccountPageHandler::RegisterInitialize(
    const std::string& email,
    const std::string& blinded_message,
    RegisterInitializeCallback callback) {
  brave_account_service_->RegisterInitialize(email, blinded_message,
                                             std::move(callback));
}

void BraveAccountPageHandler::RegisterFinalize(
    const std::string& verification_token,
    const std::string& serialized_record,
    RegisterFinalizeCallback callback) {
  brave_account_service_->RegisterFinalize(
      verification_token, serialized_record, std::move(callback));
}
