/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/brave_account_service.h"

#include <string>
#include <utility>

#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/logging.h"
#include "brave/components/brave_account/endpoints/password/finalize.h"
#include "brave/components/brave_account/endpoints/password/init.h"
#include "brave/components/brave_account/pref_names.h"
#include "components/prefs/pref_service.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_account {

BraveAccountService::BraveAccountService(
    PrefService* pref_service,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory)
    : pref_service_(pref_service),
      url_loader_factory_(url_loader_factory),
      password_init_(
          std::make_unique<endpoints::PasswordInit>(url_loader_factory_)),
      password_finalize_(
          std::make_unique<endpoints::PasswordFinalize>(url_loader_factory_)) {}

BraveAccountService::~BraveAccountService() = default;

void BraveAccountService::RegisterInitialize(
    const std::string& email,
    const std::string& blinded_message,
    base::OnceCallback<void(const std::string&)> callback) {
  password_init_->Send(
      email, blinded_message,
      base::BindOnce(&BraveAccountService::OnRegisterInitialize,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveAccountService::RegisterFinalize(
    const std::string& serialized_record,
    base::OnceCallback<void(bool)> callback) {
  std::string verification_token = pref_service_->GetString(kVerificationToken);
  password_finalize_->Send(
      verification_token, serialized_record,
      base::BindOnce(&BraveAccountService::OnRegisterFinalize,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveAccountService::OnRegisterInitialize(
    base::OnceCallback<void(const std::string&)> callback,
    const std::string& serialized_response) {
  // Parse verification token from the response and save it to preferences
  if (!serialized_response.empty()) {
    auto parsed_response = base::JSONReader::Read(serialized_response);
    if (parsed_response && parsed_response->is_dict()) {
      const std::string* verification_token =
          parsed_response->GetDict().FindString("verificationToken");
      if (verification_token && !verification_token->empty()) {
        pref_service_->SetString(kVerificationToken, *verification_token);
      }

      // Extract the serialized response for the callback
      const std::string* response =
          parsed_response->GetDict().FindString("serializedResponse");
      if (response) {
        std::move(callback).Run(*response);
        return;
      }
    }
  }

  std::move(callback).Run(serialized_response);
}

void BraveAccountService::OnRegisterFinalize(
    base::OnceCallback<void(bool)> callback,
    bool success) {
  std::move(callback).Run(success);
}

}  // namespace brave_account
