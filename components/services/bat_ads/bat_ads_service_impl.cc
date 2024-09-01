/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ads/bat_ads_service_impl.h"

#include <memory>
#include <utility>

#include "brave/components/services/bat_ads/bat_ads_impl.h"
#include "brave/components/services/bat_ads/public/interfaces/bat_ads.mojom.h"
#include "mojo/public/cpp/bindings/sync_call_restrictions.h"

namespace bat_ads {

struct BatAdsServiceImpl::ScopedAllowSyncCall {
  // TODO(https://github.com/brave/brave-browser/issues/29870): Get rid of
  // scoped allow sync calls object when Brave Ads [Sync] mojom calls are
  // refactored. mojo::ScopedAllowSyncCallForTesting is used to avoid patching
  // of chromium sync_call_restrictions.h file.
  mojo::ScopedAllowSyncCallForTesting scoped_allow_sync_call;
};

BatAdsServiceImpl::BatAdsServiceImpl()
    : scoped_allow_sync_call_(std::make_unique<ScopedAllowSyncCall>()) {}

BatAdsServiceImpl::BatAdsServiceImpl(mojo::PendingReceiver<mojom::BatAdsService>
                                         bat_ads_service_pending_receiver)
    : bat_ads_service_receiver_(this,
                                std::move(bat_ads_service_pending_receiver)) {}

BatAdsServiceImpl::~BatAdsServiceImpl() = default;

void BatAdsServiceImpl::Create(
    mojo::PendingAssociatedRemote<mojom::BatAdsClient>
        bat_ads_client_pending_associated_remote,
    mojo::PendingAssociatedReceiver<mojom::BatAds>
        bat_ads_pending_associated_receiver,
    mojo::PendingReceiver<mojom::BatAdsClientNotifier>
        bat_ads_client_notifier_pending_receiver,
    CreateCallback callback) {
  DCHECK(bat_ads_pending_associated_receiver.is_valid());

  bat_ads_associated_receivers_.Add(
      std::make_unique<BatAdsImpl>(
          std::move(bat_ads_client_pending_associated_remote),
          std::move(bat_ads_client_notifier_pending_receiver)),
      std::move(bat_ads_pending_associated_receiver));

  std::move(callback).Run();
}

}  // namespace bat_ads
