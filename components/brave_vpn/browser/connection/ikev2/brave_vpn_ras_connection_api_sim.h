/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_IKEV2_BRAVE_VPN_RAS_CONNECTION_API_SIM_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_IKEV2_BRAVE_VPN_RAS_CONNECTION_API_SIM_H_

#include <optional>
#include <string>

#include "base/gtest_prod_util.h"
#include "base/memory/weak_ptr.h"
#include "base/no_destructor.h"
#include "brave/components/brave_vpn/browser/connection/brave_vpn_connection_info.h"
#include "brave/components/brave_vpn/browser/connection/ikev2/brave_vpn_ras_connection_api_base.h"

namespace brave_vpn {

class BraveVPNOSConnectionAPISim : public BraveVPNOSConnectionAPIBase {
 public:
  BraveVPNOSConnectionAPISim(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      PrefService* local_prefs);
  ~BraveVPNOSConnectionAPISim() override;

  BraveVPNOSConnectionAPISim(const BraveVPNOSConnectionAPISim&) = delete;
  BraveVPNOSConnectionAPISim& operator=(const BraveVPNOSConnectionAPISim&) =
      delete;

  bool IsConnectionCreated() const;
  bool IsConnectionChecked() const;
  void SetNetworkAvailableForTesting(bool value);

 protected:
  friend class base::NoDestructor<BraveVPNOSConnectionAPISim>;

  // BraveVPNOSConnectionAPI overrides:
  void Connect() override;
  void Disconnect() override;
  void CheckConnection() override;

  // BraveVPNOSConnectionAPIBase interfaces:
  void CreateVPNConnectionImpl(const BraveVPNConnectionInfo& info) override;
  void ConnectImpl(const std::string& name) override;
  void DisconnectImpl(const std::string& name) override;
  void CheckConnectionImpl(const std::string& name) override;
  bool IsPlatformNetworkAvailable() override;

 private:
  friend class BraveVPNServiceTest;

  FRIEND_TEST_ALL_PREFIXES(BraveVPNOSConnectionAPIUnitTest,
                           CreateOSVPNEntryWithValidInfoWhenConnectTest);
  FRIEND_TEST_ALL_PREFIXES(BraveVPNOSConnectionAPIUnitTest,
                           CreateOSVPNEntryWithInvalidInfoTest);
  FRIEND_TEST_ALL_PREFIXES(BraveVPNServiceTest,
                           ConnectionStateUpdateWithPurchasedStateTest);
  FRIEND_TEST_ALL_PREFIXES(BraveVPNServiceTest, ResetConnectionStateTest);
  FRIEND_TEST_ALL_PREFIXES(BraveVPNServiceTest, DisconnectedIfDisabledByPolicy);

  void OnCreated(const std::string& name, bool success);
  void OnConnected(const std::string& name, bool success);
  void OnIsConnecting(const std::string& name);
  void OnDisconnected(const std::string& name, bool success);
  void OnIsDisconnecting(const std::string& name);
  void OnRemoved(const std::string& name, bool success);

  bool disconnect_requested_ = false;
  bool connection_created_ = false;
  bool check_connection_called_ = false;

  std::optional<bool> network_available_;
  base::WeakPtrFactory<BraveVPNOSConnectionAPISim> weak_factory_{this};
};

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_IKEV2_BRAVE_VPN_RAS_CONNECTION_API_SIM_H_
