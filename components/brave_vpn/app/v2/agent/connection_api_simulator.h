/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_CONNECTION_API_SIMULATOR_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_CONNECTION_API_SIMULATOR_H_

#include "base/memory/weak_ptr.h"
#include "base/timer/timer.h"
#include "brave/components/brave_vpn/app/v2/agent/connection_api.h"

namespace brave_vpn::v2 {

class ConnectionApiSimulator : public ConnectionApi {
 public:
  explicit ConnectionApiSimulator(Delegate* delegate);
  ~ConnectionApiSimulator() override;

  ConnectionApiSimulator(const ConnectionApiSimulator&) = delete;
  ConnectionApiSimulator& operator=(const ConnectionApiSimulator&) = delete;

 protected:
  // ConnectionApi:
  void ConnectImpl() override;
  void DisconnectImpl() override;
  void CheckConnectionImpl() override;

 private:
  void CancelPendingTransitions();
  void NotifyEstablished();
  void NotifyTearingDown();
  void NotifyTerminated();
  void NotifyFailed();

  bool connection_alive_ = false;
  base::OneShotTimer transition_timer_;
  base::WeakPtrFactory<ConnectionApiSimulator> weak_factory_{this};
};

}  // namespace brave_vpn::v2

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_APP_V2_AGENT_CONNECTION_API_SIMULATOR_H_
