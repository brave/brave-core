/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/filter_list_service.h"

#include <utility>

#include <string>
#include "base/feature_list.h"
#include "brave/components/brave_shields/browser/ad_block_regional_service_manager.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/common/features.h"
#include "brave/components/brave_shields/common/filter_list.mojom-forward.h"
#include "brave/components/brave_shields/common/filter_list.mojom-shared.h"
#include "brave/components/brave_shields/common/pref_names.h"
#include "components/prefs/pref_service.h"

namespace brave_shields {

FilterListService::FilterListService(AdBlockService* ad_block_service)
    : ad_block_service_(ad_block_service) {}

FilterListService::~FilterListService() = default;

mojo::PendingRemote<mojom::FilterListAndroidHandler>
FilterListService::MakeRemote() {
  mojo::PendingRemote<mojom::FilterListAndroidHandler> remote;
  receivers_.Add(this, remote.InitWithNewPipeAndPassReceiver());
  return remote;
}

void FilterListService::Bind(
    mojo::PendingReceiver<mojom::FilterListAndroidHandler> receiver) {
  receivers_.Add(this, std::move(receiver));
}

void FilterListService::IsFilterListEnabled(
    const std::string& filterListUuid,
    IsFilterListEnabledCallback callback) {
  std::move(callback).Run(
      ad_block_service_->regional_service_manager()->IsFilterListEnabled(
          filterListUuid));
}

void FilterListService::EnableFilter(const std::string& filterListUuid,
                                     bool shouldEnableFilter) {
  ad_block_service_->regional_service_manager()->EnableFilterList(
      filterListUuid, shouldEnableFilter);
}

}  // namespace brave_shields
