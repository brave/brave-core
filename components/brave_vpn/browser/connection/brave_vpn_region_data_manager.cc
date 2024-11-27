/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_vpn/browser/connection/brave_vpn_region_data_manager.h"

#include <algorithm>
#include <optional>
#include <utility>

#include "base/check_is_test.h"
#include "base/functional/bind.h"
#include "base/functional/callback_forward.h"
#include "base/json/json_reader.h"
#include "brave/components/brave_vpn/browser/api/brave_vpn_api_helper.h"
#include "brave/components/brave_vpn/browser/connection/brave_vpn_region_data_helper.h"
#include "brave/components/brave_vpn/common/brave_vpn_constants.h"
#include "brave/components/brave_vpn/common/brave_vpn_utils.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#include "components/prefs/pref_service.h"

namespace brave_vpn {
BraveVPNRegionDataManager::BraveVPNRegionDataManager(
    scoped_refptr<network::SharedURLLoaderFactory> url_loader_factory,
    PrefService* local_prefs)
    : url_loader_factory_(url_loader_factory), local_prefs_(local_prefs) {
  LoadCachedRegionData();
}

BraveVPNRegionDataManager::~BraveVPNRegionDataManager() = default;

const std::vector<mojom::RegionPtr>& BraveVPNRegionDataManager::GetRegions()
    const {
  return regions_;
}

bool BraveVPNRegionDataManager::IsRegionDataReady() const {
  return !regions_.empty();
}

void BraveVPNRegionDataManager::SetSelectedRegion(std::string_view name) {
  local_prefs_->SetString(prefs::kBraveVPNSelectedRegionV2, name);

  if (selected_region_changed_callback_) {
    selected_region_changed_callback_.Run(GetSelectedRegion());
  }
}

std::string BraveVPNRegionDataManager::GetSelectedRegion() const {
  if (regions_.empty()) {
    CHECK_IS_TEST();
  }

  auto region_name = local_prefs_->GetString(prefs::kBraveVPNSelectedRegionV2);
  if (region_name.empty()) {
    // Gives device region if there is no cached selected region.
    VLOG(2) << __func__ << " : give device region instead.";
    region_name = GetDeviceRegion();
  }

  DCHECK(!region_name.empty());
  return region_name;
}

std::string BraveVPNRegionDataManager::GetDeviceRegion() const {
  return local_prefs_->GetString(prefs::kBraveVPNDeviceRegion);
}

void BraveVPNRegionDataManager::SetDeviceRegion(std::string_view name) {
  local_prefs_->SetString(prefs::kBraveVPNDeviceRegion, name);
}

std::string BraveVPNRegionDataManager::GetRegionPrecisionForName(
    const std::string& name) const {
  for (const auto& region : regions_) {
    if (region->name == name) {
      return brave_vpn::mojom::kRegionPrecisionCountry;
    } else {
      for (const auto& city : region->cities) {
        if (city->name == name) {
          return brave_vpn::mojom::kRegionPrecisionCity;
        }
      }
    }
  }
  NOTREACHED();
}

void BraveVPNRegionDataManager::SetFallbackDeviceRegion() {
  // Set first item in the region list as a |device_region_| as a fallback.
  DCHECK(!regions_.empty());
  SetDeviceRegion(regions_[0]->name);
}

void BraveVPNRegionDataManager::SetDeviceRegionWithTimezone(
    const base::Value::List& timezones_value) {
  const std::string current_time_zone = GetCurrentTimeZone();
  if (current_time_zone.empty()) {
    return;
  }

  for (const auto& timezones : timezones_value) {
    DCHECK(timezones.is_dict());
    if (!timezones.is_dict()) {
      continue;
    }

    const std::string* region_name = timezones.GetDict().FindString("name");
    if (!region_name) {
      continue;
    }
    const auto* timezone_list_value = timezones.GetDict().FindList("timezones");
    if (!timezone_list_value) {
      continue;
    }

    for (const auto& timezone : *timezone_list_value) {
      DCHECK(timezone.is_string());
      if (!timezone.is_string()) {
        continue;
      }
      if (current_time_zone == timezone.GetString()) {
        VLOG(2) << "Found default region: " << *region_name;
        // Get new region name as timezone data could use old name.
        std::string_view new_name =
            GetMigratedNameIfNeeded(local_prefs_, *region_name);
        SetDeviceRegion(new_name);
        // Use device region as a default selected region.
        if (local_prefs_->GetString(prefs::kBraveVPNSelectedRegionV2).empty()) {
          SetSelectedRegion(new_name);
        }
        return;
      }
    }
  }
}

void BraveVPNRegionDataManager::LoadCachedRegionData() {
  // Already loaded from cache.
  if (!regions_.empty()) {
    return;
  }

  // Empty device region means it's initial state.
  if (GetDeviceRegion().empty()) {
    return;
  }

  auto* preference = local_prefs_->FindPreference(prefs::kBraveVPNRegionList);
  DCHECK(preference);
  // Early return when we don't have any cached region data.
  if (preference->IsDefaultValue()) {
    return;
  }

  // If cached one is outdated, don't use it.
  if (!ValidateCachedRegionData(preference->GetValue()->GetList())) {
    VLOG(2) << __func__ << " : Cached data is outdate. Will get fetch latest.";
    return;
  }

  if (ParseAndCacheRegionList(preference->GetValue()->GetList())) {
    VLOG(2) << __func__ << " : Loaded cached region list";
    return;
  }

  VLOG(2) << __func__ << " : Failed to load cached region list";
}

bool BraveVPNRegionDataManager::NeedToUpdateRegionData() const {
  if (!IsRegionDataReady()) {
    return true;
  }

  // Skip checking region data update when we have cached one and its age is
  // younger than 5h.
  const auto last_fetched_date =
      local_prefs_->GetTime(prefs::kBraveVPNRegionListFetchedDate);
  constexpr int kRegionDataFetchIntervalInHours = 5;

  if (last_fetched_date.is_null() ||
      (base::Time::Now() - last_fetched_date).InHours() >=
          kRegionDataFetchIntervalInHours) {
    return true;
  }

  return false;
}

void BraveVPNRegionDataManager::NotifyRegionDataReady() const {
  if (region_data_ready_callback_) {
    region_data_ready_callback_.Run(!regions_.empty());
  }
}

void BraveVPNRegionDataManager::FetchRegionDataIfNeeded() {
  if (api_request_) {
    VLOG(2) << __func__ << " : Region data fetching is in-progress";
    return;
  }

  if (!NeedToUpdateRegionData()) {
    VLOG(2)
        << __func__
        << " : Don't need to check as it's not passed 5h since the last check.";
    NotifyRegionDataReady();
    return;
  }
  FetchRegions();
}

void BraveVPNRegionDataManager::FetchRegions() {
  api_request_ = std::make_unique<BraveVpnAPIRequest>(url_loader_factory_);
  VLOG(2) << __func__ << " : Start fetching region data";
  // Unretained is safe here becasue this class owns |api_request_|.
  api_request_->GetServerRegions(
      base::BindOnce(&BraveVPNRegionDataManager::OnFetchRegionList,
                     base::Unretained(this)),
      mojom::kRegionPrecisionCityByCountry);
}

void BraveVPNRegionDataManager::OnFetchRegionList(
    const std::string& region_list,
    bool success) {
  if (!api_request_) {
    CHECK_IS_TEST();
  }
  api_request_.reset();
  std::optional<base::Value> value = base::JSONReader::Read(region_list);
  if (value && value->is_list() &&
      ParseAndCacheRegionList(value->GetList(), true)) {
    VLOG(2) << "Got valid region list";
    // Set default device region and it'll be updated when received valid
    // timezone info.
    SetFallbackDeviceRegion();
    // Fetch timezones list to determine default region of this device.
    api_request_ = std::make_unique<BraveVpnAPIRequest>(url_loader_factory_);
    api_request_->GetTimezonesForRegions(base::BindOnce(
        &BraveVPNRegionDataManager::OnFetchTimezones, base::Unretained(this)));
    return;
  }

  VLOG(2) << "Got invalid region list";
  NotifyRegionDataReady();
}

bool BraveVPNRegionDataManager::ParseAndCacheRegionList(
    const base::Value::List& region_value,
    bool save_to_prefs) {
  auto new_regions = ParseRegionList(region_value);
  VLOG(2) << __func__ << " : has regionlist: " << !new_regions.empty();

  // To avoid deleting current valid |regions_|, only assign when
  // |new_regions| is not empty.
  if (new_regions.empty()) {
    return false;
  }

  regions_ = std::move(new_regions);

  if (save_to_prefs) {
    SetRegionListToPrefs();
  }
  return true;
}

void BraveVPNRegionDataManager::OnFetchTimezones(
    const std::string& timezones_list,
    bool success) {
  api_request_.reset();

  std::optional<base::Value> value = base::JSONReader::Read(timezones_list);
  if (success && value && value->is_list()) {
    VLOG(2) << "Got valid timezones list";
    SetDeviceRegionWithTimezone(value->GetList());
  } else {
    VLOG(2) << "Failed to get invalid timezones list";
  }

  // Can notify as ready now regardless of timezone fetching result.
  // We use default one picked from region list as a device region on failure.
  NotifyRegionDataReady();
}

void BraveVPNRegionDataManager::SetRegionListToPrefs() {
  DCHECK(!regions_.empty());

  base::Value::List regions_list;
  for (const auto& region : regions_) {
    regions_list.Append(GetValueFromRegion(region));
  }

  local_prefs_->Set(prefs::kBraveVPNRegionList,
                    base::Value(std::move(regions_list)));
  local_prefs_->SetTime(prefs::kBraveVPNRegionListFetchedDate,
                        base::Time::Now());
}

std::string BraveVPNRegionDataManager::GetCurrentTimeZone() {
  if (!test_timezone_.empty()) {
    return test_timezone_;
  }

  return GetTimeZoneName();
}

}  // namespace brave_vpn
