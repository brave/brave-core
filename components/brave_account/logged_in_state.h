/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_LOGGED_IN_STATE_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_LOGGED_IN_STATE_H_

#include <string>

#include "base/check_is_test.h"
#include "base/memory/scoped_refptr.h"
#include "base/timer/timer.h"
#include "brave/components/brave_account/brave_account_state_prefs.h"
#include "brave/components/brave_account/flows/cancel_verification.h"
#include "brave/components/brave_account/flows/change_password.h"
#include "brave/components/brave_account/flows/get_service_token.h"
#include "brave/components/brave_account/flows/log_out.h"
#include "brave/components/brave_account/flows/resend_verification_email.h"
#include "brave/components/brave_account/flows/update_email.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "brave/components/brave_account/state_base.h"
#include "components/os_crypt/async/common/encryptor.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace brave_account {

// `mojom::Authentication` surface available after login: the log-out,
// password-change, and service-token steps, which are delegated to the
// `log_out_`, `change_password_`, and `get_service_token_` helpers. The
// `update_email_` helper additionally drives itself, periodically refreshing
// the stored email in the background. `ResendVerificationEmail()` and
// `CancelVerification()` are valid in both states, so this state owns its own
// `resend_verification_email_` and `cancel_verification_` helpers. All other
// methods inherit `StateBase`'s wrong-state default.
class LoggedInState : public StateBase {
 public:
  LoggedInState(
      AccountStatePrefs& account_state_prefs,
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      const os_crypt_async::Encryptor& encryptor,
      AddObserverCallback add_observer);

  LoggedInState(const LoggedInState&) = delete;
  LoggedInState& operator=(const LoggedInState&) = delete;

  ~LoggedInState() override;

  base::OneShotTimer& update_email_timer_for_testing() {
    CHECK_IS_TEST();
    return update_email_.timer_for_testing();  // IN-TEST
  }

 private:
  void CancelVerification(mojom::VerificationIntentPtr intent) override;

  void ChangePasswordStep1(const std::string& email,
                           ChangePasswordStep1Callback callback) override;

  void ChangePasswordStep2(const std::string& code,
                           ChangePasswordStep2Callback callback) override;

  void ChangePasswordStep3(const std::string& blinded_message,
                           ChangePasswordStep3Callback callback) override;

  void ChangePasswordStep4(const std::string& serialized_record,
                           ChangePasswordStep4Callback callback) override;

  void GetServiceToken(mojom::Service service,
                       GetServiceTokenCallback callback) override;

  void LogOut() override;

  void ResendVerificationEmail(
      mojom::VerificationIntentPtr intent,
      ResendVerificationEmailCallback callback) override;

  class CancelVerification cancel_verification_;
  ChangePassword change_password_;
  class GetServiceToken get_service_token_;
  class LogOut log_out_;
  class ResendVerificationEmail resend_verification_email_;
  UpdateEmail update_email_;
};

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_LOGGED_IN_STATE_H_
