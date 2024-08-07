/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_BRAVE_VPN_REGION_DATA_MANAGER_H_
#define BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_BRAVE_VPN_REGION_DATA_MANAGER_H_

#include <memory>
#include <string>
#include <vector>

#include "base/functional/callback_forward.h"
#include "base/memory/raw_ptr.h"
#include "base/memory/scoped_refptr.h"
#include "brave/components/brave_vpn/browser/api/brave_vpn_api_request.h"
#include "brave/components/brave_vpn/common/mojom/brave_vpn.mojom.h"
#include "services/network/public/cpp/shared_url_loader_factory.h"

class PrefService;

namespace brave_vpn {

class BraveVPNRegionDataManager {
 public:
  BraveVPNRegionDataManager(
      scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
      PrefService* local_prefs);
  ~BraveVPNRegionDataManager();
  BraveVPNRegionDataManager(const BraveVPNRegionDataManager&) = delete;
  BraveVPNRegionDataManager& operator=(const BraveVPNRegionDataManager&) =
      delete;

  const std::vector<mojom::RegionPtr>& GetRegions() const;
  bool IsRegionDataReady() const;
  std::string GetSelectedRegion() const;
  void FetchRegionDataIfNeeded();
  void FetchRegions();

  void set_selected_region_changed_callback(
      base::RepeatingCallback<void(const std::string&)> callback) {
    selected_region_changed_callback_ = callback;
  }

  void set_region_data_ready_callback(
      base::RepeatingCallback<void(bool)> callback) {
    region_data_ready_callback_ = callback;
  }
  void SetSelectedRegion(const std::string& name);
  std::string GetDeviceRegion() const;

 private:
  friend class BraveVPNServiceTest;
  friend class SystemVPNConnectionAPIUnitTest;

  void SetDeviceRegion(const std::string& name);

  void SetFallbackDeviceRegion();
  void SetDeviceRegionWithTimezone(const base::Value::List& timezons_value);

  void LoadCachedRegionData();
  void OnFetchRegionList(const std::string& region_list, bool success);
  bool ParseAndCacheRegionList(const base::Value::List& region_value,
                               bool save_to_prefs = false);
  void OnFetchTimezones(const std::string& timezones_list, bool success);
  void SetRegionListToPrefs();

  // Notify it's ready when |regions_| is not empty.
  std::string GetCurrentTimeZone();
  bool NeedToUpdateRegionData() const;
  void NotifyRegionDataReady() const;

  // For testing only.
  std::string test_timezone_;

  std::vector<mojom::RegionPtr> regions_;

  // Only not null when region_data fetching is in-progress.
  scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory_;
  raw_ptr<PrefService> local_prefs_ = nullptr;
  std::unique_ptr<BraveVpnAPIRequest> api_request_;
  base::RepeatingCallback<void(const std::string&)>
      selected_region_changed_callback_;
  base::RepeatingCallback<void(bool)> region_data_ready_callback_;
};

}  // namespace brave_vpn

#endif  // BRAVE_COMPONENTS_BRAVE_VPN_BROWSER_CONNECTION_BRAVE_VPN_REGION_DATA_MANAGER_H_
