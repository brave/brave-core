/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/state_base.h"

#include <utility>

#include "base/check.h"
#include "base/check_deref.h"
#include "base/functional/bind.h"
#include "base/types/expected.h"
#include "brave/components/brave_account/brave_account_encryption.h"
#include "brave/components/brave_account/brave_account_service_constants.h"
#include "brave/components/brave_account/endpoint_client/with_headers.h"
#include "brave/components/brave_account/endpoints/verify_delete.h"
#include "brave/components/brave_account/state_internal.h"
#include "net/http/http_status_code.h"

namespace brave_account {

using endpoint_client::SetBearerToken;
using endpoint_client::WithHeaders;
using endpoints::VerifyDelete;
using endpoints::VerifyResend;
using internal::MakeCalledInWrongStateError;
using internal::MakeClientError;
using internal::MakeRequest;
using internal::MakeServerError;

void StateBase::AddReceiver(
    mojo::PendingReceiver<mojom::Authentication> receiver) {
  receivers_.Add(this, std::move(receiver));
}

std::vector<mojo::PendingReceiver<mojom::Authentication>>
StateBase::TakeReceivers() {
  return receivers_.TakeReceivers();
}

StateBase::StateBase(
    AccountStatePrefs& account_state_prefs,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    const os_crypt_async::Encryptor& encryptor,
    AddObserverCallback add_observer)
    : account_state_prefs_(account_state_prefs),
      url_loader_factory_(std::move(url_loader_factory)),
      encryption_(encryptor),
      add_observer_(std::move(add_observer)) {
  CHECK(url_loader_factory_);
  CHECK(add_observer_);
}

StateBase::~StateBase() = default;

std::string StateBase::Encrypt(const std::string& plain_text) const {
  return encryption_.Encrypt(plain_text);
}

std::string StateBase::Decrypt(const std::string& base64) const {
  return encryption_.Decrypt(base64);
}

void StateBase::AddObserver(
    mojo::PendingRemote<mojom::AuthenticationObserver> observer) {
  add_observer_.Run(std::move(observer));
}

void StateBase::RegisterInitialize(mojom::Service,
                                   const std::string&,
                                   const std::string&,
                                   RegisterInitializeCallback callback) {
  std::move(callback).Run(MakeCalledInWrongStateError<mojom::RegisterError>());
}

void StateBase::RegisterFinalize(const std::string&,
                                 const std::string&,
                                 RegisterFinalizeCallback callback) {
  std::move(callback).Run(MakeCalledInWrongStateError<mojom::RegisterError>());
}

void StateBase::RegisterVerify(const std::string&,
                               RegisterVerifyCallback callback) {
  std::move(callback).Run(MakeCalledInWrongStateError<mojom::RegisterError>());
}

void StateBase::ResendVerificationEmail(
    mojom::VerificationIntentPtr intent,
    ResendVerificationEmailCallback callback) {
  auto verification_token =
      GetDecryptedVerificationToken<mojom::ResendConfirmationEmailError>(
          std::move(intent));
  if (!verification_token.has_value()) {
    return std::move(callback).Run(
        base::unexpected(std::move(verification_token).error()));
  }

  auto request = MakeRequest<WithHeaders<VerifyResend::Request>>();
  SetBearerToken(request, *verification_token);
  // Server side will determine locale based on the Accept-Language request
  // header (which is included automatically by upstream).
  request.body.locale = "";
  request.timeout_duration = kVerifyResendTimeout;

  SendStateOwnedRequest<VerifyResend>(
      std::move(request),
      base::BindOnce(&StateBase::OnResendVerificationEmail,
                     weak_factory_.GetWeakPtr(), std::move(callback)));
}

void StateBase::CancelVerification(mojom::VerificationIntentPtr intent) {
  CHECK(intent);

  if (intent->is_logged_out_intent() &&
      intent->get_logged_out_intent() ==
          mojom::LoggedOutVerificationIntent::kRegistration) {
    // Best-effort notification to the server, since server side will clean up
    // verification tokens automatically (currently after 30 minutes).
    // Not adopted into the state's in-flight bag:
    // best-effort with no callback that touches state.
    if (const auto verification_token =
            GetDecryptedVerificationToken(std::move(intent));
        !verification_token.empty()) {
      auto request = MakeRequest<WithHeaders<VerifyDelete::Request>>();
      SetBearerToken(request, verification_token);

      SendUnownedRequest<VerifyDelete>(std::move(request));
    }
  }

  // LoggedOutWithVerification ==> LoggedOut (no state swap), or
  // LoggedInWithVerification ==> LoggedIn (no state swap).
  account_state_prefs_->ClearVerification();
}

void StateBase::ResetPasswordVerifyInit(
    mojom::Service initiating_service,
    const std::string& email,
    ResetPasswordVerifyInitCallback callback) {
  std::move(callback).Run(
      MakeCalledInWrongStateError<mojom::ResetPasswordError>());
}

void StateBase::ResetPasswordVerifyComplete(
    const std::string& code,
    ResetPasswordVerifyCompleteCallback callback) {
  std::move(callback).Run(
      MakeCalledInWrongStateError<mojom::ResetPasswordError>());
}

void StateBase::ResetPasswordPasswordInit(
    mojom::Service initiating_service,
    const std::string& blinded_message,
    ResetPasswordPasswordInitCallback callback) {
  std::move(callback).Run(
      MakeCalledInWrongStateError<mojom::ResetPasswordError>());
}

void StateBase::ResetPasswordPasswordFinalize(
    const std::string& serialized_record,
    const std::string& email,
    ResetPasswordPasswordFinalizeCallback callback) {
  std::move(callback).Run(
      MakeCalledInWrongStateError<mojom::ResetPasswordError>());
}

void StateBase::LoginInitialize(mojom::Service,
                                const std::string&,
                                const std::string&,
                                LoginInitializeCallback callback) {
  std::move(callback).Run(MakeCalledInWrongStateError<mojom::LoginError>());
}

void StateBase::LoginFinalize(const std::string&,
                              const std::string&,
                              LoginFinalizeCallback callback) {
  std::move(callback).Run(MakeCalledInWrongStateError<mojom::LoginError>());
}

void StateBase::LogOut() {}

void StateBase::GetServiceToken(mojom::Service,
                                GetServiceTokenCallback callback) {
  std::move(callback).Run(
      MakeCalledInWrongStateError<mojom::GetServiceTokenError>());
}

void StateBase::OnResendVerificationEmail(
    ResendVerificationEmailCallback callback,
    VerifyResend::Response response) {
  if (response.status_code == net::HTTP_NO_CONTENT) {
    return std::move(callback).Run(mojom::ResendConfirmationEmailResult::New());
  }

  if (!response.body || response.body->has_value()) {
    return std::move(callback).Run(
        base::unexpected(MakeServerError<mojom::ResendConfirmationEmailError>(
            response.status_code.value_or(response.net_error),
            mojom::ResendConfirmationEmailServerErrorCode::kInvalidResponse)));
  }

  std::move(callback).Run(
      base::unexpected(MakeServerError<mojom::ResendConfirmationEmailError>(
          CHECK_DEREF(response.status_code),
          std::move(response.body->error()))));
}

void StateBase::RemoveRequestHandle(
    std::list<endpoint_client::RequestHandle>::iterator slot) {
  in_flight_.erase(slot);
}

}  // namespace brave_account
