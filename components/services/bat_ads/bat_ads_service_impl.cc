/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ads/bat_ads_service_impl.h"

#include <memory>
#include <utility>

#include "brave/components/brave_ads/common/interfaces/ads.mojom.h"
#include "brave/components/brave_ads/core/build_channel.h"
#include "brave/components/brave_ads/core/sys_info.h"
#include "brave/components/services/bat_ads/bat_ads_impl.h"

namespace bat_ads {

BatAdsServiceImpl::BatAdsServiceImpl(
    mojo::PendingReceiver<mojom::BatAdsService> receiver)
    : receiver_(this, std::move(receiver)) {}

BatAdsServiceImpl::~BatAdsServiceImpl() = default;

void BatAdsServiceImpl::Create(
    mojo::PendingAssociatedRemote<mojom::BatAdsClient> client_info,
    mojo::PendingAssociatedReceiver<mojom::BatAds> bat_ads,
    CreateCallback callback) {
  associated_receivers_.Add(
      std::make_unique<BatAdsImpl>(std::move(client_info)), std::move(bat_ads));

  std::move(callback).Run();
}

void BatAdsServiceImpl::SetSysInfo(brave_ads::mojom::SysInfoPtr sys_info,
                                   SetSysInfoCallback callback) {
  brave_ads::SysInfo().device_id = sys_info->device_id;
  brave_ads::SysInfo().is_uncertain_future = sys_info->is_uncertain_future;

  std::move(callback).Run();
}

void BatAdsServiceImpl::SetBuildChannel(
    brave_ads::mojom::BuildChannelInfoPtr build_channel,
    SetBuildChannelCallback callback) {
  brave_ads::BuildChannel().is_release = build_channel->is_release;
  brave_ads::BuildChannel().name = build_channel->name;

  std::move(callback).Run();
}

}  // namespace bat_ads
