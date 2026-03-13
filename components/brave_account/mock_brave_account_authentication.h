// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_MOCK_BRAVE_ACCOUNT_AUTHENTICATION_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_MOCK_BRAVE_ACCOUNT_AUTHENTICATION_H_

#include <optional>
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
              RegisterInitialize,
              (std::optional<mojom::Service> initiating_service,
               const std::string& email,
               const std::string& blinded_message,
               RegisterInitializeCallback callback),
              (override));

  MOCK_METHOD(void,
              RegisterFinalize,
              (const std::string& encrypted_verification_token,
               const std::string& serialized_record,
               RegisterFinalizeCallback callback),
              (override));

  MOCK_METHOD(void,
              ResendConfirmationEmail,
              (ResendConfirmationEmailCallback callback),
              (override));

  MOCK_METHOD(void, CancelRegistration, (), (override));

  MOCK_METHOD(void,
              LoginInitialize,
              (std::optional<mojom::Service> initiating_service,
               const std::string& email,
               const std::string& serialized_ke1,
               LoginInitializeCallback callback),
              (override));

  MOCK_METHOD(void,
              LoginFinalize,
              (const std::string& encrypted_login_token,
               const std::string& client_mac,
               LoginFinalizeCallback callback),
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
