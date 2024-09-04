/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_federated/eligibility_service.h"
#include "brave/components/brave_federated/eligibility_service_observer.h"

namespace brave_federated {

EligibilityService::EligibilityService()
    : is_on_battery_power_(
          base::PowerMonitor::GetInstance()
              ->AddPowerStateObserverAndReturnOnBatteryState(this)) {
  connection_type_ = net::NetworkChangeNotifier::GetConnectionType();
  net::NetworkChangeNotifier::AddNetworkChangeObserver(this);
}

EligibilityService::~EligibilityService() {
  base::PowerMonitor::GetInstance()->RemovePowerStateObserver(this);
  net::NetworkChangeNotifier::RemoveNetworkChangeObserver(this);
}

void EligibilityService::AddObserver(Observer* observer) {
  DCHECK(observer);

  observers_.AddObserver(observer);
}

void EligibilityService::RemoveObserver(Observer* observer) {
  DCHECK(observer);

  observers_.RemoveObserver(observer);
}

void EligibilityService::NotifyObservers(bool is_eligible) {
  for (auto& observer : observers_) {
    observer.OnEligibilityChanged(is_eligible);
  }
}

bool EligibilityService::IsEligibile() const {
  return !is_on_battery_power_ && IsConnectedToWifiOrEthernet();
}

///////////////////////////////////////////////////////////////////////////////

void EligibilityService::MaybeChangeEligibility() {
  if (is_eligible_ == IsEligibile()) {
    return;
  }

  is_eligible_ = !is_eligible_;
  NotifyObservers(is_eligible_);
}

bool EligibilityService::IsConnectedToWifiOrEthernet() const {
  return connection_type_ == net::NetworkChangeNotifier::CONNECTION_WIFI ||
         connection_type_ == net::NetworkChangeNotifier::CONNECTION_ETHERNET;
}

void EligibilityService::OnPowerStateChange(bool on_battery_power) {
  is_on_battery_power_ = on_battery_power;
  MaybeChangeEligibility();
}

void EligibilityService::OnNetworkChanged(
    net::NetworkChangeNotifier::ConnectionType type) {
  connection_type_ = type;
  MaybeChangeEligibility();
}

}  // namespace brave_federated
