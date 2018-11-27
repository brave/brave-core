/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/services/bat_ads/bat_ads_service_impl.h"

#include <memory>
#include <utility>

#include "brave/components/services/bat_ads/bat_ads_impl.h"
#include "mojo/public/cpp/bindings/strong_associated_binding.h"

namespace bat_ads {

BatAdsServiceImpl::BatAdsServiceImpl(
    std::unique_ptr<service_manager::ServiceContextRef> service_ref)
    : service_ref_(std::move(service_ref)) {}

BatAdsServiceImpl::~BatAdsServiceImpl() {}

// Overridden from BatAdsService:
void BatAdsServiceImpl::Create(
    mojom::BatAdsClientAssociatedPtrInfo client_info,
    mojom::BatAdsAssociatedRequest bat_ads,
    CreateCallback callback) {
  mojo::MakeStrongAssociatedBinding(
      std::make_unique<BatAdsImpl>(std::move(client_info)), std::move(bat_ads));
  std::move(callback).Run();
}

}  // namespace bat_ads
