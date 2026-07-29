/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_LOGGED_OUT_STATE_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_LOGGED_OUT_STATE_H_

#include <string>

#include "base/memory/scoped_refptr.h"
#include "brave/components/brave_account/brave_account_state_prefs.h"
#include "brave/components/brave_account/flows/login.h"
#include "brave/components/brave_account/flows/register.h"
#include "brave/components/brave_account/flows/reset_password.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "brave/components/brave_account/state_base.h"
#include "components/os_crypt/async/common/encryptor.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace brave_account {

// `mojom::Authentication` surface available before login:
// the login, registration, and password-reset steps,
// which are delegated to the `login_`, `register_`, and
// `reset_password_` helpers.
// `ResendVerificationEmail()` and `CancelVerification()` are fully
// handled by `StateBase` for both states.
// All other methods inherit `StateBase`'s wrong-state default.
class LoggedOutState : public StateBase {
 public:
  LoggedOutState(
      AccountStatePrefs& account_state_prefs,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      const os_crypt_async::Encryptor& encryptor,
      AddObserverCallback add_observer);

  LoggedOutState(const LoggedOutState&) = delete;
  LoggedOutState& operator=(const LoggedOutState&) = delete;

  ~LoggedOutState() override;

 private:
  void LoginStep1(mojom::Service initiating_service,
                  const std::string& email,
                  const std::string& serialized_ke1,
                  LoginStep1Callback callback) override;

  void LoginStep2(const std::string& encrypted_login_token,
                  const std::string& client_mac,
                  LoginStep2Callback callback) override;

  void RegisterStep1(mojom::Service initiating_service,
                     const std::string& email,
                     const std::string& blinded_message,
                     RegisterStep1Callback callback) override;

  void RegisterStep2(const std::string& encrypted_verification_token,
                     const std::string& serialized_record,
                     RegisterStep2Callback callback) override;

  void RegisterStep3(const std::string& code,
                     RegisterStep3Callback callback) override;

  void ResetPasswordStep1(const std::string& email,
                          ResetPasswordStep1Callback callback) override;

  void ResetPasswordStep2(const std::string& code,
                          ResetPasswordStep2Callback callback) override;

  void ResetPasswordStep3(const std::string& blinded_message,
                          ResetPasswordStep3Callback callback) override;

  void ResetPasswordStep4(const std::string& serialized_record,
                          const std::string& email,
                          ResetPasswordStep4Callback callback) override;

  Login login_{*this};
  Register register_{*this};
  ResetPassword reset_password_{*this};
};

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_LOGGED_OUT_STATE_H_
