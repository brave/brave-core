/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ads/bat_ads_service_impl.h"

#include <memory>
#include <utility>

#include "bat/ads/ads.h"
#include "brave/components/services/bat_ads/bat_ads_impl.h"

namespace bat_ads {

BatAdsServiceImpl::BatAdsServiceImpl(
    mojo::PendingReceiver<mojom::BatAdsService> receiver)
    : receiver_(this, std::move(receiver)),
      is_initialized_(false) {}

BatAdsServiceImpl::~BatAdsServiceImpl() {}

void BatAdsServiceImpl::Create(
    mojo::PendingAssociatedRemote<mojom::BatAdsClient> client_info,
    mojo::PendingAssociatedReceiver<mojom::BatAds> bat_ads,
    CreateCallback callback) {

  associated_receivers_.Add(
      std::make_unique<BatAdsImpl>(std::move(client_info)),
      std::move(bat_ads));
  is_initialized_ = true;
  std::move(callback).Run();
}

void BatAdsServiceImpl::SetEnvironment(
    const ads::mojom::Environment environment,
    SetEnvironmentCallback callback) {
  DCHECK(!is_initialized_);
  ads::g_environment = environment;
  std::move(callback).Run();
}

void BatAdsServiceImpl::SetSysInfo(ads::mojom::SysInfoPtr sys_info,
                                   SetSysInfoCallback callback) {
  DCHECK(!is_initialized_);
  ads::g_sys_info.is_uncertain_future = sys_info->is_uncertain_future;
  std::move(callback).Run();
}

void BatAdsServiceImpl::SetBuildChannel(
    ads::mojom::BuildChannelPtr build_channel,
    SetBuildChannelCallback callback) {
  DCHECK(!is_initialized_);
  ads::g_build_channel.is_release = build_channel->is_release;
  ads::g_build_channel.name = build_channel->name;
  std::move(callback).Run();
}

void BatAdsServiceImpl::SetDebug(
    const bool is_debug,
    SetDebugCallback callback) {
  DCHECK(!is_initialized_);
  ads::g_is_debug = is_debug;
  std::move(callback).Run();
}

}  // namespace bat_ads
