/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/units/promoted_content_ad/promoted_content_ad_handler.h"

#include <utility>

#include "base/check.h"
#include "brave/components/brave_ads/core/internal/account/account.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/history/history_manager.h"
#include "brave/components/brave_ads/core/internal/settings/settings.h"
#include "brave/components/brave_ads/core/internal/transfer/transfer.h"
#include "brave/components/brave_ads/core/internal/units/promoted_content_ad/promoted_content_ad_info.h"
#include "brave/components/brave_ads/core/public/account/confirmations/confirmation_type.h"

namespace brave_ads {

namespace {

void FireEventCallback(
    TriggerAdEventCallback callback,
    const bool success,
    const std::string& /*placement_id=*/,
    const mojom::PromotedContentAdEventType /*event_type=*/) {
  std::move(callback).Run(success);
}

}  // namespace

PromotedContentAdHandler::PromotedContentAdHandler(Account& account,
                                                   Transfer& transfer)
    : account_(account), transfer_(transfer) {
  event_handler_.SetDelegate(this);
}

PromotedContentAdHandler::~PromotedContentAdHandler() = default;

void PromotedContentAdHandler::TriggerEvent(
    const std::string& placement_id,
    const std::string& creative_instance_id,
    const mojom::PromotedContentAdEventType event_type,
    TriggerAdEventCallback callback) {
  CHECK(mojom::IsKnownEnumValue(event_type));
  CHECK_NE(mojom::PromotedContentAdEventType::kServed, event_type)
      << "Should not be called with kServed as this event is handled when "
         "calling TriggerEvent with kViewed";

  if (!UserHasOptedInToBraveNewsAds()) {
    return std::move(callback).Run(/*success=*/false);
  }

  if (event_type == mojom::PromotedContentAdEventType::kViewed) {
    return event_handler_.FireEvent(
        placement_id, creative_instance_id,
        mojom::PromotedContentAdEventType::kServed,
        base::BindOnce(&PromotedContentAdHandler::TriggerServedEventCallback,
                       weak_factory_.GetWeakPtr(), creative_instance_id,
                       std::move(callback)));
  }

  event_handler_.FireEvent(
      placement_id, creative_instance_id, event_type,
      base::BindOnce(&FireEventCallback, std::move(callback)));
}

///////////////////////////////////////////////////////////////////////////////

void PromotedContentAdHandler::TriggerServedEventCallback(
    const std::string& creative_instance_id,
    TriggerAdEventCallback callback,
    const bool success,
    const std::string& placement_id,
    const mojom::PromotedContentAdEventType /*event_type=*/) {
  if (!success) {
    return std::move(callback).Run(/*success=*/false);
  }

  event_handler_.FireEvent(
      placement_id, creative_instance_id,
      mojom::PromotedContentAdEventType::kViewed,
      base::BindOnce(&FireEventCallback, std::move(callback)));
}

void PromotedContentAdHandler::OnDidFirePromotedContentAdServedEvent(
    const PromotedContentAdInfo& ad) {
  BLOG(3, "Served promoted content ad with placement id "
              << ad.placement_id << " and creative instance id "
              << ad.creative_instance_id);
}

void PromotedContentAdHandler::OnDidFirePromotedContentAdViewedEvent(
    const PromotedContentAdInfo& ad) {
  BLOG(3, "Viewed promoted content ad with placement id "
              << ad.placement_id << " and creative instance id "
              << ad.creative_instance_id);

  HistoryManager::GetInstance().Add(ad, ConfirmationType::kViewed);

  account_->Deposit(ad.creative_instance_id, ad.segment, ad.type,
                    ConfirmationType::kViewed);
}

void PromotedContentAdHandler::OnDidFirePromotedContentAdClickedEvent(
    const PromotedContentAdInfo& ad) {
  BLOG(3, "Clicked promoted content ad with placement id "
              << ad.placement_id << " and creative instance id "
              << ad.creative_instance_id);

  transfer_->SetLastClickedAd(ad);

  HistoryManager::GetInstance().Add(ad, ConfirmationType::kClicked);

  account_->Deposit(ad.creative_instance_id, ad.segment, ad.type,
                    ConfirmationType::kClicked);
}

}  // namespace brave_ads
