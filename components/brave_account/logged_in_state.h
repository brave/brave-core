/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_LOGGED_IN_STATE_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_LOGGED_IN_STATE_H_

#include <string>

#include "base/check_is_test.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/components/brave_account/brave_account_state_prefs.h"
#include "brave/components/brave_account/endpoint_client/request_handle.h"
#include "brave/components/brave_account/endpoints/auth_validate.h"
#include "brave/components/brave_account/endpoints/service_token.h"
#include "brave/components/brave_account/mojom/brave_account.mojom.h"
#include "brave/components/brave_account/state_base.h"
#include "components/os_crypt/async/common/encryptor.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace brave_account {

// `mojom::Authentication` surface available after login: `LogOut()` and
// `GetServiceToken()`. Also periodically refreshes the stored email via
// `ScheduleAuthValidate()`/`AuthValidate()`, driven by `auth_validate_timer_`.
// All other methods inherit `StateBase`'s wrong-state default.
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

  base::OneShotTimer& auth_validate_timer_for_testing() {
    CHECK_IS_TEST();
    return auth_validate_timer_;
  }

 private:
  void LogOut() override;

  void GetServiceToken(mojom::Service service,
                       GetServiceTokenCallback callback) override;

  void OnGetServiceToken(const std::string& service_name,
                         GetServiceTokenCallback callback,
                         endpoints::ServiceToken::Response response);

  void ScheduleAuthValidate(
      base::TimeDelta delay = base::Seconds(0),
      endpoint_client::RequestHandle current_auth_validate_request = {});

  void AuthValidate(
      endpoint_client::RequestHandle current_auth_validate_request);

  void OnAuthValidate(endpoints::AuthValidate::Response response);

  base::OneShotTimer auth_validate_timer_;
  base::WeakPtrFactory<LoggedInState> weak_factory_{this};
};

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_LOGGED_IN_STATE_H_
