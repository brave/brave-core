/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_TEST_FAKE_CONNECTION_API_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_TEST_FAKE_CONNECTION_API_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/raw_ref.h"
#include "brave/components/brave_vpn/app/v2/agent/connection_api.h"

namespace brave_vpn::v2 {

// A ConnectionApi that does nothing on its own. Tests look at the requests it
// received in its Log and answer them by hand with the Report*() methods, the
// way a real implementation would: later, never from inside the request.
class FakeConnectionApi : public ConnectionApi {
 public:
  // Outlives the API instances, which come and go.
  struct Log {
    int created = 0;
    int destroyed = 0;
    int connect_requests = 0;
    int disconnect_requests = 0;
    int check_requests = 0;
    // The live instance, if any.
    raw_ptr<FakeConnectionApi> current = nullptr;
  };

  FakeConnectionApi(Delegate* delegate, Log* log);
  ~FakeConnectionApi() override;

  FakeConnectionApi(const FakeConnectionApi&) = delete;
  FakeConnectionApi& operator=(const FakeConnectionApi&) = delete;

  void ReportEstablished();
  void ReportTearingDown();
  void ReportTerminated();
  void ReportFailed(FailureReason reason);

 protected:
  // ConnectionApi:
  void ConnectImpl() override;
  void DisconnectImpl() override;
  void CheckConnectionImpl() override;

 private:
  const raw_ref<Log> log_;
};

}  // namespace brave_vpn::v2

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_TEST_FAKE_CONNECTION_API_H_
