/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/cookie_list_opt_in_service.h"

#include <utility>

#include "base/feature_list.h"
#include "brave/components/brave_shields/browser/ad_block_regional_service_manager.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/components/brave_shields/common/cookie_list_opt_in.mojom-forward.h"
#include "brave/components/brave_shields/common/cookie_list_opt_in.mojom-shared.h"
#include "brave/components/brave_shields/common/features.h"

namespace brave_shields {
CookieListOptInService::CookieListOptInService(AdBlockService* ad_block_service)
    : ad_block_service_(ad_block_service) {}

CookieListOptInService::~CookieListOptInService() = default;

mojo::PendingRemote<mojom::CookieListOptInPageAndroidHandler>
CookieListOptInService::MakeRemote() {
  mojo::PendingRemote<mojom::CookieListOptInPageAndroidHandler> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void CookieListOptInService::Bind(
    mojo::PendingReceiver<mojom::CookieListOptInPageAndroidHandler> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void CookieListOptInService::ShouldShowDialog(
    ShouldShowDialogCallback callback) {
  bool should_show_dialog =
      base::FeatureList::IsEnabled(
          brave_shields::features::kBraveAdblockCookieListOptIn) &&
      ad_block_service_->regional_service_manager()->IsFilterListAvailable(
          brave_shields::kCookieListUuid);
  std::move(callback).Run(should_show_dialog);
}

void CookieListOptInService::IsFilterListEnabled(
    IsFilterListEnabledCallback callback) {
  std::move(callback).Run(
      ad_block_service_->regional_service_manager()->IsFilterListEnabled(
          brave_shields::kCookieListUuid));
}

void CookieListOptInService::EnableFilter(bool shouldEnableFilter) {
  ad_block_service_->regional_service_manager()->EnableFilterList(
      brave_shields::kCookieListUuid, shouldEnableFilter);
}

}  // namespace brave_shields
