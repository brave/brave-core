/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_ELIGIBILITY_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_ELIGIBILITY_SERVICE_H_

#include "base/power_monitor/power_monitor.h"
#include "net/base/network_change_notifier.h"

namespace brave_federated {

class EligibilityService
    : public base::PowerStateObserver,
      public net::NetworkChangeNotifier::NetworkChangeObserver {
 public:
  EligibilityService();
  ~EligibilityService() override;

  EligibilityService(const EligibilityService&) = delete;
  EligibilityService& operator=(const EligibilityService&) = delete;

  bool IsEligibileForFederatedTask();

 private:
  // base::PowerStateObserver
  void OnPowerStateChange(bool on_battery_power) override;

  // net::NetworkChangeNotifier::NetworkChangeObserver overrides.
  void OnNetworkChanged(
      net::NetworkChangeNotifier::ConnectionType type) override;

  bool IsConnectionWifiOrEthernet();

  bool is_on_battery_power_;
  net::NetworkChangeNotifier::ConnectionType connection_type_;
};

}  // namespace brave_federated

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_ELIGIBILITY_SERVICE_H_
