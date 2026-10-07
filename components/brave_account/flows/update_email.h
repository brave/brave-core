/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_UPDATE_EMAIL_H_
#define BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_UPDATE_EMAIL_H_

#include "base/check_is_test.h"
#include "base/memory/scoped_refptr.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "brave/components/brave_account/brave_account_state_prefs.h"
#include "brave/components/brave_account/endpoint_client/request_handle.h"
#include "brave/components/brave_account/endpoints/auth_validate.h"
#include "brave/components/brave_account/flows/flow_base.h"

namespace network {
class SharedURLLoaderFactory;
}  // namespace network

namespace os_crypt_async {
class Encryptor;
}  // namespace os_crypt_async

namespace brave_account {

// Owns the periodic email refresh of the logged-in `mojom::Authentication`
// surface. Unlike the other flow helpers, this one is not driven by a mojom
// method: `LoggedInState` holds an `UpdateEmail` member and this flow drives
// itself, polling one backend endpoint on a timer starting at construction:
//
//   -> /v2/auth/validate
//
// A successful response refreshes the stored email; a 4xx forces logged-out
// (and stops polling). Each poll is a caller-owned request whose handle is
// passed forward into the next scheduled poll, which cancels any still-pending
// previous attempt before issuing the new one (see
// `FlowBase::SendCallerOwnedRequest`).
class UpdateEmail : public FlowBase {
 public:
  UpdateEmail(AccountStatePrefs& account_state_prefs,
              scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
              const os_crypt_async::Encryptor& encryptor);

  UpdateEmail(const UpdateEmail&) = delete;
  UpdateEmail& operator=(const UpdateEmail&) = delete;

  ~UpdateEmail();

  base::OneShotTimer& timer_for_testing() {
    CHECK_IS_TEST();
    return timer_;
  }

 private:
  void ScheduleRequest(base::TimeDelta delay = base::Seconds(0),
                       endpoint_client::RequestHandle current_request = {});

  void SendRequest(endpoint_client::RequestHandle current_request);

  void OnResponse(endpoints::AuthValidate::Response response);

  base::OneShotTimer timer_;
  base::WeakPtrFactory<UpdateEmail> weak_factory_{this};
};

}  // namespace brave_account

#endif  // BRAVE_COMPONENTS_BRAVE_ACCOUNT_FLOWS_UPDATE_EMAIL_H_
