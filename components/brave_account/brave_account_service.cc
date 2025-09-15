/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/brave_account_service.h"

#include <utility>

#include "base/notimplemented.h"
#include "components/prefs/pref_service.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_account {

BraveAccountService::BraveAccountService(
    PrefService* pref_service,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : pref_service_(pref_service), url_loader_factory_(url_loader_factory) {}

BraveAccountService::~BraveAccountService() = default;

void BraveAccountService::RegisterInitialize(
    const std::string& email,
    const std::string& blinded_message,
    mojom::Authentication::RegisterInitializeCallback callback) {
  // TODO(https://github.com/brave/brave-browser/issues/47400)
  // TODO(https://github.com/brave/brave-browser/issues/48488)
  NOTIMPLEMENTED();
  std::move(callback).Run(mojom::RegisterInitializeResult::New());
}

void BraveAccountService::RegisterFinalize(
    const std::string& encrypted_verification_token,
    const std::string& serialized_record,
    mojom::Authentication::RegisterFinalizeCallback callback) {
  // TODO(https://github.com/brave/brave-browser/issues/47400)
  // TODO(https://github.com/brave/brave-browser/issues/48488)
  NOTIMPLEMENTED();
  std::move(callback).Run(mojom::RegisterFinalizeResult::New());
}

}  // namespace brave_account
