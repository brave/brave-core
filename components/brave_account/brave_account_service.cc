/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/brave_account_service.h"

#include <string>
#include <utility>

#include "base/functional/bind.h"
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
  password_finalize_->Send(
      pref_service_->GetString(kVerificationToken), serialized_record,
      base::BindOnce(&BraveAccountService::OnRegisterFinalize,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveAccountService::OnRegisterInitialize(
    base::OnceCallback<void(const std::string&)> callback,
    api_request_helper::APIRequestResult result) {
  auto& body = result.value_body();
  DVLOG(0) << result.response_code();
  DVLOG(0) << body;

  if (!result.Is2XXResponseCode() || !body.is_dict()) {
    return std::move(callback).Run("");
  }

  const std::string* verification_token =
      body.GetDict().FindString("verificationToken");
  const std::string* serialized_response =
      body.GetDict().FindString("serializedResponse");

  if (!verification_token || verification_token->empty() ||
      !serialized_response || serialized_response->empty()) {
    return std::move(callback).Run("");
  }

  pref_service_->SetString(kVerificationToken, *verification_token);
  std::move(callback).Run(*serialized_response);
}

void BraveAccountService::OnRegisterFinalize(
    base::OnceCallback<void(bool)> callback,
    api_request_helper::APIRequestResult result) {
  DVLOG(0) << result.response_code();
  DVLOG(0) << result.value_body();
  std::move(callback).Run(result.Is2XXResponseCode());
}

}  // namespace brave_account
