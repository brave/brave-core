/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/logged_out_state.h"

#include <utility>

#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_account {

LoggedOutState::LoggedOutState(
    AccountStatePrefs& account_state_prefs,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    const os_crypt_async::Encryptor& encryptor,
    AddObserverCallback add_observer)
    : StateBase(account_state_prefs,
                std::move(url_loader_factory),
                encryptor,
                std::move(add_observer)) {}

LoggedOutState::~LoggedOutState() = default;

void LoggedOutState::LoginStep1(mojom::Service initiating_service,
                                const std::string& email,
                                const std::string& serialized_ke1,
                                LoginStep1Callback callback) {
  login_.Step1(initiating_service, email, serialized_ke1, std::move(callback));
}

void LoggedOutState::LoginStep2(const std::string& encrypted_login_token,
                                const std::string& client_mac,
                                LoginStep2Callback callback) {
  login_.Step2(encrypted_login_token, client_mac, std::move(callback));
}

void LoggedOutState::RegisterPasswordInit(
    mojom::Service initiating_service,
    const std::string& email,
    const std::string& blinded_message,
    RegisterPasswordInitCallback callback) {
  register_.PasswordInit(initiating_service, email, blinded_message,
                         std::move(callback));
}

void LoggedOutState::RegisterPasswordFinalize(
    const std::string& encrypted_verification_token,
    const std::string& serialized_record,
    RegisterPasswordFinalizeCallback callback) {
  register_.PasswordFinalize(encrypted_verification_token, serialized_record,
                             std::move(callback));
}

void LoggedOutState::RegisterVerifyComplete(
    const std::string& code,
    RegisterVerifyCompleteCallback callback) {
  register_.VerifyComplete(code, std::move(callback));
}

void LoggedOutState::ResetPasswordVerifyInit(
    const std::string& email,
    ResetPasswordVerifyInitCallback callback) {
  reset_password_.VerifyInit(email, std::move(callback));
}

void LoggedOutState::ResetPasswordVerifyComplete(
    const std::string& code,
    ResetPasswordVerifyCompleteCallback callback) {
  reset_password_.VerifyComplete(code, std::move(callback));
}

void LoggedOutState::ResetPasswordPasswordInit(
    const std::string& blinded_message,
    ResetPasswordPasswordInitCallback callback) {
  reset_password_.PasswordInit(blinded_message, std::move(callback));
}

void LoggedOutState::ResetPasswordPasswordFinalize(
    const std::string& serialized_record,
    const std::string& email,
    ResetPasswordPasswordFinalizeCallback callback) {
  reset_password_.PasswordFinalize(serialized_record, email,
                                   std::move(callback));
}

}  // namespace brave_account
