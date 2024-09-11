/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_FEDERATED_ELIGIBILITY_SERVICE_H_
#define BRAVE_COMPONENTS_BRAVE_FEDERATED_ELIGIBILITY_SERVICE_H_

#include "base/power_monitor/power_monitor.h"
#include "net/base/network_change_notifier.h"

namespace brave_federated {

class Observer;

// Certain classes of federated tasks might be computationally and bandwidth
// expensive to run on the client. For these classes we require the client's
// device to be attached to power and on a Wi-Fi connection. EligibilityService
// monitors the device battery and power state to determine if the device is
// eligible to run federated tasks.
class EligibilityService
    : public base::PowerStateObserver,
      public net::NetworkChangeNotifier::NetworkChangeObserver {
 public:
  EligibilityService();
  ~EligibilityService() override;

  EligibilityService(const EligibilityService&) = delete;
  EligibilityService& operator=(const EligibilityService&) = delete;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);
  void NotifyObservers(bool is_eligible);

  bool IsEligibile() const;

 private:
  void MaybeChangeEligibility();

  bool IsConnectedToWifiOrEthernet() const;

  // base::PowerStateObserver
  void OnBatteryPowerStatusChange(base::PowerStateObserver::BatteryPowerStatus
                                      battery_power_status) override;

  // net::NetworkChangeNotifier::NetworkChangeObserver overrides.
  void OnNetworkChanged(
      net::NetworkChangeNotifier::ConnectionType type) override;

  base::ObserverList<Observer> observers_;
  bool is_eligible_ = false;
  bool is_on_battery_power_;
  net::NetworkChangeNotifier::ConnectionType connection_type_;
};

}  // namespace brave_federated

#endif  // BRAVE_COMPONENTS_BRAVE_FEDERATED_ELIGIBILITY_SERVICE_H_
