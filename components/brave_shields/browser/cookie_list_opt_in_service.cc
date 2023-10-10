/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_shields/browser/cookie_list_opt_in_service.h"

#include <utility>

#include "base/feature_list.h"
#include "base/metrics/histogram_functions.h"
#include "brave/components/brave_shields/browser/ad_block_regional_service_manager.h"
#include "brave/components/brave_shields/browser/ad_block_service.h"
#include "brave/components/brave_shields/common/brave_shield_constants.h"
#include "brave/components/brave_shields/common/cookie_list_opt_in.mojom-forward.h"
#include "brave/components/brave_shields/common/cookie_list_opt_in.mojom-shared.h"
#include "brave/components/brave_shields/common/features.h"
#include "brave/components/brave_shields/common/pref_names.h"
#include "components/prefs/pref_service.h"

namespace brave_shields {

CookieListOptInService::CookieListOptInService(AdBlockService* ad_block_service,
                                               PrefService* local_state)
    : ad_block_service_(ad_block_service), local_state_(local_state) {
  if (base::FeatureList::IsEnabled(
          brave_shields::features::kBraveAdblockCookieListOptIn)) {
    if (!local_state->GetBoolean(prefs::kAdBlockCookieListOptInShown)) {
      base::UmaHistogramExactLinear(kCookieListPromptHistogram, 0, 4);
    }
  }
}

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

void CookieListOptInService::OnTooltipShown() {
  local_state_->SetBoolean(prefs::kAdBlockCookieListOptInShown, true);
  base::UmaHistogramExactLinear(kCookieListPromptHistogram, 1, 4);
}

void CookieListOptInService::OnTooltipNoClicked() {
  base::UmaHistogramExactLinear(kCookieListPromptHistogram, 2, 4);
}

void CookieListOptInService::OnTooltipYesClicked() {
  base::UmaHistogramExactLinear(kCookieListPromptHistogram, 3, 4);
}

}  // namespace brave_shields
