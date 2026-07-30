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

void LoggedOutState::RegisterStep1(mojom::Service initiating_service,
                                   const std::string& email,
                                   const std::string& blinded_message,
                                   RegisterStep1Callback callback) {
  register_.Step1(initiating_service, email, blinded_message,
                  std::move(callback));
}

void LoggedOutState::RegisterStep2(
    const std::string& encrypted_verification_token,
    const std::string& serialized_record,
    RegisterStep2Callback callback) {
  register_.Step2(encrypted_verification_token, serialized_record,
                  std::move(callback));
}

void LoggedOutState::RegisterStep3(const std::string& code,
                                   RegisterStep3Callback callback) {
  register_.Step3(code, std::move(callback));
}

void LoggedOutState::ResetPasswordStep1(const std::string& email,
                                        ResetPasswordStep1Callback callback) {
  reset_password_.Step1(email, std::move(callback));
}

void LoggedOutState::ResetPasswordStep2(const std::string& code,
                                        ResetPasswordStep2Callback callback) {
  reset_password_.Step2(code, std::move(callback));
}

void LoggedOutState::ResetPasswordStep3(const std::string& blinded_message,
                                        ResetPasswordStep3Callback callback) {
  reset_password_.Step3(blinded_message, std::move(callback));
}

void LoggedOutState::ResetPasswordStep4(const std::string& serialized_record,
                                        const std::string& email,
                                        ResetPasswordStep4Callback callback) {
  reset_password_.Step4(serialized_record, email, std::move(callback));
}

}  // namespace brave_account
