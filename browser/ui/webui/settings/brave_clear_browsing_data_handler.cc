// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/settings/brave_clear_browsing_data_handler.h"

#include <utility>

#include "base/check.h"
#include "base/check_op.h"
#include "base/functional/bind.h"
#include "base/values.h"
#include "brave/components/brave_rewards/core/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"

#if BUILDFLAG(ENABLE_BRAVE_ADS)
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/components/brave_ads/core/browser/service/ads_service.h"
#endif

namespace settings {

BraveClearBrowsingDataHandler::BraveClearBrowsingDataHandler(
    content::WebUI* webui,
    Profile* profile)
    : ClearBrowsingDataHandler(webui, profile), profile_(profile) {
  CHECK(profile_);

  pref_change_registrar_.Init(profile_->GetPrefs());
  pref_change_registrar_.Add(
      brave_rewards::prefs::kEnabled,
      base::BindRepeating(
          &BraveClearBrowsingDataHandler::OnRewardsEnabledPreferenceChanged,
          base::Unretained(this)));
}

BraveClearBrowsingDataHandler::~BraveClearBrowsingDataHandler() = default;

void BraveClearBrowsingDataHandler::RegisterMessages() {
  ClearBrowsingDataHandler::RegisterMessages();

  web_ui()->RegisterMessageCallback(
      "getBraveRewardsEnabled",
      base::BindRepeating(
          &BraveClearBrowsingDataHandler::HandleGetBraveRewardsEnabled,
          base::Unretained(this)));
#if BUILDFLAG(ENABLE_BRAVE_ADS)
  web_ui()->RegisterMessageCallback(
      "clearBraveAdsData",
      base::BindRepeating(
          &BraveClearBrowsingDataHandler::HandleClearBraveAdsData,
          base::Unretained(this)));
#endif  // BUILDFLAG(ENABLE_BRAVE_ADS)
}

void BraveClearBrowsingDataHandler::HandleGetBraveRewardsEnabled(
    const base::ListValue& args) {
  CHECK_EQ(args.size(), 1U);

  const bool rewards_enabled =
      profile_->GetPrefs()->GetBoolean(brave_rewards::prefs::kEnabled);

  AllowJavascript();
  ResolveJavascriptCallback(args[0], rewards_enabled);
}

#if BUILDFLAG(ENABLE_BRAVE_ADS)
void BraveClearBrowsingDataHandler::HandleClearBraveAdsData(
    const base::ListValue& args) {
  CHECK_EQ(args.size(), 1U);

  AllowJavascript();

  base::Value callback_id = args[0].Clone();

  auto* const ads_service =
      brave_ads::AdsServiceFactory::GetForProfile(profile_);
  if (!ads_service) {
    OnClearBraveAdsDataComplete(std::move(callback_id), /*success=*/false);
    return;
  }

  ads_service->ClearData(base::BindOnce(
      &BraveClearBrowsingDataHandler::OnClearBraveAdsDataComplete,
      weak_ptr_factory_.GetWeakPtr(), std::move(callback_id)));
}

void BraveClearBrowsingDataHandler::OnClearBraveAdsDataComplete(
    base::Value callback_id,
    bool success) {
  if (!IsJavascriptAllowed()) {
    return;
  }

  ResolveJavascriptCallback(callback_id, base::Value(success));
}
#endif  // BUILDFLAG(ENABLE_BRAVE_ADS)

void BraveClearBrowsingDataHandler::OnRewardsEnabledPreferenceChanged() {
  if (!IsJavascriptAllowed()) {
    return;
  }

  const bool rewards_enabled =
      profile_->GetPrefs()->GetBoolean(brave_rewards::prefs::kEnabled);
  FireWebUIListener("brave-rewards-enabled-changed",
                    base::Value(rewards_enabled));
}

}  // namespace settings
