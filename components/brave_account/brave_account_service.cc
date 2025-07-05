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
    mojom::PageHandler::RegisterInitializeCallback callback) {
  DCHECK(!email.empty());
  DCHECK(!blinded_message.empty());

  password_init_->Send(
      email, blinded_message,
      base::BindOnce(&BraveAccountService::OnRegisterInitialize,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void BraveAccountService::RegisterFinalize(
    const std::string& verification_token,
    const std::string& serialized_record,
    mojom::PageHandler::RegisterFinalizeCallback callback) {
  DCHECK(!verification_token.empty());
  DCHECK(!serialized_record.empty());

  password_finalize_->Send(
      verification_token, serialized_record,
      base::BindOnce(&BraveAccountService::OnRegisterFinalize,
                     weak_factory_.GetWeakPtr(), std::move(callback),
                     verification_token));
}

void BraveAccountService::OnRegisterInitialize(
    mojom::PageHandler::RegisterInitializeCallback callback,
    api_request_helper::APIRequestResult result) {
  auto& body = result.value_body();
  DVLOG(0) << result.response_code();
  DVLOG(0) << body;

  switch (result.response_code()) {
    case 200:
      break;
    case 400:
      return std::move(callback).Run(base::unexpected(
          mojom::RegisterFailureReason::kInitializeBadRequest));
    case 401:
      return std::move(callback).Run(base::unexpected(
          mojom::RegisterFailureReason::kInitializeUnauthorized));
    case 403:
      return std::move(callback).Run(
          base::unexpected(mojom::RegisterFailureReason::kInitializeForbidden));
    case 500:
      return std::move(callback).Run(base::unexpected(
          mojom::RegisterFailureReason::kInitializeInternalServerError));
    default:
      return std::move(callback).Run(
          base::unexpected(mojom::RegisterFailureReason::kInitializeUnknown));
  }

  if (!body.is_dict()) {
    return std::move(callback).Run(
        base::unexpected(mojom::RegisterFailureReason::kInitializeUnexpected));
  }

  const std::string* verification_token =
      body.GetDict().FindString("verificationToken");
  const std::string* serialized_response =
      body.GetDict().FindString("serializedResponse");

  if (!verification_token || verification_token->empty() ||
      !serialized_response || serialized_response->empty()) {
    return std::move(callback).Run(
        base::unexpected(mojom::RegisterFailureReason::kInitializeUnexpected));
  }

  std::move(callback).Run(mojom::RegisterInitializeResult::New(
      *verification_token, *serialized_response));
}

void BraveAccountService::OnRegisterFinalize(
    mojom::PageHandler::RegisterFinalizeCallback callback,
    const std::string& verification_token,
    api_request_helper::APIRequestResult result) {
  DVLOG(0) << result.response_code();
  DVLOG(0) << result.value_body();

  switch (result.response_code()) {
    case 200:
      pref_service_->SetString(kVerificationToken, verification_token);
      return std::move(callback).Run(mojo_base::mojom::Empty::New());
    case 400:
      return std::move(callback).Run(
          base::unexpected(mojom::RegisterFailureReason::kFinalizeBadRequest));
    case 401:
      return std::move(callback).Run(base::unexpected(
          mojom::RegisterFailureReason::kFinalizeUnauthorized));
    case 403:
      return std::move(callback).Run(
          base::unexpected(mojom::RegisterFailureReason::kFinalizeForbidden));
    case 404:
      return std::move(callback).Run(
          base::unexpected(mojom::RegisterFailureReason::kFinalizeNotFound));
    case 500:
      return std::move(callback).Run(base::unexpected(
          mojom::RegisterFailureReason::kFinalizeInternalServerError));
    default:
      return std::move(callback).Run(
          base::unexpected(mojom::RegisterFailureReason::kFinalizeUnknown));
  }
}

}  // namespace brave_account
