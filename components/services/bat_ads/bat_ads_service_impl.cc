/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ads/bat_ads_service_impl.h"

#include <memory>
#include <utility>

#include "bat/ads/ads.h"
#include "brave/components/services/bat_ads/bat_ads_impl.h"
#include "mojo/public/cpp/bindings/strong_associated_binding.h"

namespace bat_ads {

BatAdsServiceImpl::BatAdsServiceImpl(
    std::unique_ptr<service_manager::ServiceContextRef> service_ref)
    : service_ref_(std::move(service_ref)),
      has_initialized_(false) {}

BatAdsServiceImpl::~BatAdsServiceImpl() {}

void BatAdsServiceImpl::Create(
    mojom::BatAdsClientAssociatedPtrInfo client_info,
    mojom::BatAdsAssociatedRequest bat_ads,
    CreateCallback callback) {
  mojo::MakeStrongAssociatedBinding(
      std::make_unique<BatAdsImpl>(std::move(client_info)), std::move(bat_ads));
  has_initialized_ = true;
  std::move(callback).Run();
}

void BatAdsServiceImpl::SetProduction(
    const bool is_production,
    SetProductionCallback callback) {
  DCHECK(!has_initialized_ || ads::_is_production == is_production);
  ads::_is_production = is_production;
  std::move(callback).Run();
}

void BatAdsServiceImpl::SetTesting(
    const bool is_testing,
    SetTestingCallback callback) {
  DCHECK(!has_initialized_ || ads::_is_testing == is_testing);
  ads::_is_testing = is_testing;
  std::move(callback).Run();
}

void BatAdsServiceImpl::SetDebug(
    const bool is_debug,
    SetDebugCallback callback) {
  DCHECK(!has_initialized_ || ads::_is_debug == is_debug);
  ads::_is_debug = is_debug;
  std::move(callback).Run();
}

void BatAdsServiceImpl::IsSupportedRegion(
    const std::string& locale,
    IsSupportedRegionCallback callback) {
  DCHECK(!has_initialized_);
  std::move(callback).Run(ads::Ads::IsSupportedRegion(locale));
}

}  // namespace bat_ads
