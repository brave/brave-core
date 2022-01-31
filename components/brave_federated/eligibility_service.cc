/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/eligibility_service.h"

#include "base/power_monitor/power_monitor.h"
#include "net/base/network_change_notifier.h"

namespace brave_federated {

EligibilityService::EligibilityService()
    : is_on_battery_power_(
          base::PowerMonitor::AddPowerStateObserverAndReturnOnBatteryState(
              this)) {
  connection_type_ = net::NetworkChangeNotifier::GetConnectionType();
  net::NetworkChangeNotifier::AddNetworkChangeObserver(this);
}

EligibilityService::~EligibilityService() {}

bool EligibilityService::IsEligibileForFederatedTask() {
  return is_on_battery_power_ && IsConnectionWifiOrEthernet();
}

bool EligibilityService::IsConnectionWifiOrEthernet() {
  return connection_type_ == net::NetworkChangeNotifier::CONNECTION_WIFI ||
         connection_type_ == net::NetworkChangeNotifier::CONNECTION_ETHERNET;
}

void EligibilityService::OnPowerStateChange(bool on_battery_power) {
  is_on_battery_power_ = on_battery_power;
}

void EligibilityService::OnNetworkChanged(
    net::NetworkChangeNotifier::ConnectionType type) {
  connection_type_ = type;
}

}  // namespace brave_federated
