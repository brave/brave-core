/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_account/logged_in_state.h"

#include <utility>

#include "services/network/public/cpp/shared_url_loader_factory.h"

namespace brave_account {

LoggedInState::LoggedInState(
    AccountStatePrefs& account_state_prefs,
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    const os_crypt_async::Encryptor& encryptor,
    AddObserverCallback add_observer)
    : StateBase(std::move(add_observer)),
      cancel_verification_(account_state_prefs, url_loader_factory, encryptor),
      change_password_(account_state_prefs, url_loader_factory, encryptor),
      get_service_token_(account_state_prefs, url_loader_factory, encryptor),
      log_out_(account_state_prefs, url_loader_factory, encryptor),
      resend_verification_email_(account_state_prefs,
                                 url_loader_factory,
                                 encryptor),
      update_email_(account_state_prefs,
                    std::move(url_loader_factory),
                    encryptor) {}

LoggedInState::~LoggedInState() = default;

void LoggedInState::CancelVerification(mojom::VerificationIntentPtr intent) {
  cancel_verification_(std::move(intent));
}

void LoggedInState::ChangePasswordStep1(const std::string& email,
                                        ChangePasswordStep1Callback callback) {
  change_password_.Step1(email, std::move(callback));
}

void LoggedInState::ChangePasswordStep2(const std::string& code,
                                        ChangePasswordStep2Callback callback) {
  change_password_.Step2(code, std::move(callback));
}

void LoggedInState::ChangePasswordStep3(const std::string& blinded_message,
                                        ChangePasswordStep3Callback callback) {
  change_password_.Step3(blinded_message, std::move(callback));
}

void LoggedInState::ChangePasswordStep4(const std::string& serialized_record,
                                        ChangePasswordStep4Callback callback) {
  change_password_.Step4(serialized_record, std::move(callback));
}

void LoggedInState::GetServiceToken(mojom::Service service,
                                    GetServiceTokenCallback callback) {
  get_service_token_(service, std::move(callback));
}

void LoggedInState::LogOut() {
  log_out_();
}

void LoggedInState::ResendVerificationEmail(
    mojom::VerificationIntentPtr intent,
    ResendVerificationEmailCallback callback) {
  resend_verification_email_(std::move(intent), std::move(callback));
}

}  // namespace brave_account
