// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_MOCK_BRAVE_ACCOUNT_AUTHENTICATION_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_MOCK_BRAVE_ACCOUNT_AUTHENTICATION_H_

#include <string>

#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "mojo/public/cpp/bindings/pending_remote.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace brave_account {

class MockBraveAccountAuthentication : public mojom::Authentication {
 public:
  MockBraveAccountAuthentication();
  ~MockBraveAccountAuthentication() override;

  MOCK_METHOD(void,
              AddObserver,
              (mojo::PendingRemote<mojom::AuthenticationObserver> observer),
              (override));

  MOCK_METHOD(void,
              RegisterStep1,
              (mojom::Service initiating_service,
               const std::string& email,
               const std::string& blinded_message,
               RegisterStep1Callback callback),
              (override));

  MOCK_METHOD(void,
              RegisterStep2,
              (const std::string& encrypted_verification_token,
               const std::string& serialized_record,
               RegisterStep2Callback callback),
              (override));

  MOCK_METHOD(void,
              RegisterStep3,
              (const std::string& code, RegisterStep3Callback callback),
              (override));

  MOCK_METHOD(void,
              ResendVerificationEmail,
              (mojom::VerificationIntentPtr intent,
               ResendVerificationEmailCallback callback),
              (override));

  MOCK_METHOD(void,
              CancelVerification,
              (mojom::VerificationIntentPtr intent),
              (override));

  MOCK_METHOD(void,
              ResetPasswordStep1,
              (const std::string& email, ResetPasswordStep1Callback callback),
              (override));

  MOCK_METHOD(void,
              ResetPasswordStep2,
              (const std::string& code, ResetPasswordStep2Callback callback),
              (override));

  MOCK_METHOD(void,
              ResetPasswordStep3,
              (const std::string& blinded_message,
               ResetPasswordStep3Callback callback),
              (override));

  MOCK_METHOD(void,
              ResetPasswordStep4,
              (const std::string& serialized_record,
               const std::string& email,
               ResetPasswordStep4Callback callback),
              (override));

  MOCK_METHOD(void,
              LoginStep1,
              (mojom::Service initiating_service,
               const std::string& email,
               const std::string& serialized_ke1,
               LoginStep1Callback callback),
              (override));

  MOCK_METHOD(void,
              LoginStep2,
              (const std::string& encrypted_login_token,
               const std::string& client_mac,
               LoginStep2Callback callback),
              (override));

  MOCK_METHOD(void,
              ChangePasswordStep1,
              (const std::string& email, ChangePasswordStep1Callback callback),
              (override));

  MOCK_METHOD(void,
              ChangePasswordStep2,
              (const std::string& code, ChangePasswordStep2Callback callback),
              (override));

  MOCK_METHOD(void,
              ChangePasswordStep3,
              (const std::string& blinded_message,
               ChangePasswordStep3Callback callback),
              (override));

  MOCK_METHOD(void,
              ChangePasswordStep4,
              (const std::string& serialized_record,
               ChangePasswordStep4Callback callback),
              (override));

  MOCK_METHOD(void, LogOut, (), (override));

  MOCK_METHOD(void,
              GetServiceToken,
              (mojom::Service service, GetServiceTokenCallback callback),
              (override));

  mojo::PendingRemote<mojom::Authentication> BindAndGetRemote();

 private:
  mojo::Receiver<mojom::Authentication> receiver_{this};
};

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_MOCK_BRAVE_ACCOUNT_AUTHENTICATION_H_
