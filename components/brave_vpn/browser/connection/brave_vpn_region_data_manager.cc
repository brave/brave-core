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
#include "brave/components/brave_vpn/common/brave_vpn_constants.h"
#include "brave/components/brave_vpn/common/pref_names.h"
#include "components/prefs/pref_service.h"

namespace brave_vpn {

namespace {

base::Value::Dict GetValueFromRegionWithoutCity(
    const mojom::RegionPtr& region) {
  base::Value::Dict region_dict;
  region_dict.Set(kRegionNameKey, region->name);
  region_dict.Set(kRegionNamePrettyKey, region->name_pretty);
  region_dict.Set(kRegionContinentKey, region->continent);
  region_dict.Set(kRegionCountryIsoCodeKey, region->country_iso_code);
  region_dict.Set(kRegionPrecisionKey, region->region_precision);
  region_dict.Set(kRegionLatitudeKey, region->latitude);
  region_dict.Set(kRegionLongitudeKey, region->longitude);
  region_dict.Set(kRegionServerCountKey, region->server_count);
  return region_dict;
}

base::Value::Dict GetValueFromRegion(const mojom::RegionPtr& region) {
  base::Value::Dict region_dict = GetValueFromRegionWithoutCity(region);
  base::Value::List cities;
  for (const auto& city : region->cities) {
    cities.Append(GetValueFromRegionWithoutCity(city));
  }
  region_dict.Set(kRegionCitiesKey, std::move(cities));
  return region_dict;
}

bool IsValidRegionValue(const base::Value::Dict& value) {
  if (!value.FindString(kRegionNameKey) ||
      !value.FindString(kRegionNamePrettyKey) ||
      !value.FindString(kRegionContinentKey) ||
      !value.FindString(kRegionCountryIsoCodeKey) ||
      !value.FindString(kRegionPrecisionKey) ||
      !value.FindList(kRegionCitiesKey) ||
      !value.FindString(kRegionLatitudeKey) ||
      !value.FindString(kRegionLongitudeKey) ||
      !value.FindString(kRegionServerCountKey)) {
    return false;
  }

  return true;
}

mojom::RegionPtr GetRegionFromValueWithoutCity(const base::Value::Dict& value) {
  mojom::RegionPtr region = mojom::Region::New();
  if (auto* name = value.FindString(brave_vpn::kRegionNameKey)) {
    region->name = *name;
  }
  if (auto* name_pretty = value.FindString(brave_vpn::kRegionNamePrettyKey)) {
    region->name_pretty = *name_pretty;
  }
  if (auto* continent = value.FindString(brave_vpn::kRegionContinentKey)) {
    region->continent = *continent;
  }
  if (auto* country_iso_code =
          value.FindString(brave_vpn::kRegionCountryIsoCodeKey)) {
    region->country_iso_code = *country_iso_code;
  }
  if (auto* region_precision =
          value.FindString(brave_vpn::kRegionPrecisionKey)) {
    region->region_precision = *region_precision;
  }
  if (auto latitude = value.FindDouble(brave_vpn::kRegionLatitudeKey)) {
    region->latitude = *latitude;
  }
  if (auto longitude = value.FindDouble(brave_vpn::kRegionLongitudeKey)) {
    region->longitude = *longitude;
  }
  if (auto server_count = value.FindInt(brave_vpn::kRegionServerCountKey)) {
    region->server_count = *server_count;
  }

  return region;
}

mojom::RegionPtr GetRegionFromValue(const base::Value::Dict& value) {
  mojom::RegionPtr region = GetRegionFromValueWithoutCity(value);
  // for (const auto& city : *value.FindList(kRegionCitiesKey)) {
  //   region.cities.push_back(GetRegionFromValueWithoutCity(city.GetDict()).Clone());
  // }

  const auto* cities = value.FindList(kRegionCitiesKey);
  for (const auto& city : *cities) {
    region->cities.push_back(GetRegionFromValueWithoutCity(city.GetDict()));
  }

  return region;
}

bool ValidateCachedRegionData(const base::Value::List& region_value) {
  for (const auto& value : region_value) {
    // Make sure cached one has all latest properties.
    if (!value.is_dict() || !IsValidRegionValue(value.GetDict())) {
      return false;
    }
  }

  return true;
}

std::vector<mojom::RegionPtr> ParseRegionList(
    const base::Value::List& region_list) {
  std::vector<mojom::RegionPtr> regions;
  for (const auto& value : region_list) {
    DCHECK(value.is_dict());
    if (!value.is_dict()) {
      continue;
    }
    regions.push_back(GetRegionFromValue(value.GetDict()).Clone());
  }

  // Sort region list alphabetically
  std::sort(regions.begin(), regions.end(),
            [](mojom::RegionPtr& a, mojom::RegionPtr& b) {
              return (a->name_pretty < b->name_pretty);
            });
  return regions;
}

}  // namespace

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

void BraveVPNRegionDataManager::SetSelectedRegion(const std::string& name) {
  local_prefs_->SetString(prefs::kBraveVPNSelectedRegion, name);

  if (selected_region_changed_callback_) {
    selected_region_changed_callback_.Run(name);
  }
}

std::string BraveVPNRegionDataManager::GetSelectedRegion() const {
  DCHECK(!regions_.empty())
      << "regions data must be prepared before panel asks.";

  auto region_name = local_prefs_->GetString(prefs::kBraveVPNSelectedRegion);
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

void BraveVPNRegionDataManager::SetDeviceRegion(const std::string& name) {
  local_prefs_->SetString(prefs::kBraveVPNDeviceRegion, name);
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
        SetDeviceRegion(*region_name);
        // Use device region as a default selected region.
        if (local_prefs_->GetString(prefs::kBraveVPNSelectedRegion).empty()) {
          SetSelectedRegion(*region_name);
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

  api_request_ = std::make_unique<BraveVpnAPIRequest>(url_loader_factory_);
  VLOG(2) << __func__ << " : Start fetching region data";

  // Unretained is safe here becasue this class owns |api_request_|.
  api_request_->GetServerRegionsWithCities(base::BindOnce(
      &BraveVPNRegionDataManager::OnFetchRegionList, base::Unretained(this)));
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
