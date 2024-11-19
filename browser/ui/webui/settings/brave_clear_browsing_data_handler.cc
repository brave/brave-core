// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "brave/browser/ui/webui/settings/brave_clear_browsing_data_handler.h"

#include "base/functional/bind.h"
#include "base/functional/callback_helpers.h"
#include "brave/browser/brave_ads/ads_service_factory.h"
#include "brave/components/brave_ads/browser/ads_service.h"
#include "brave/components/brave_rewards/common/pref_names.h"
#include "chrome/browser/profiles/profile.h"
#include "components/prefs/pref_service.h"

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
  web_ui()->RegisterMessageCallback(
      "clearBraveAdsData",
      base::BindRepeating(
          &BraveClearBrowsingDataHandler::HandleClearBraveAdsData,
          base::Unretained(this)));
}

void BraveClearBrowsingDataHandler::HandleGetBraveRewardsEnabled(
    const base::Value::List& args) {
  CHECK_EQ(args.size(), 1U);

  const bool rewards_enabled =
      profile_->GetPrefs()->GetBoolean(brave_rewards::prefs::kEnabled);

  AllowJavascript();
  ResolveJavascriptCallback(args[0], rewards_enabled);
}

void BraveClearBrowsingDataHandler::HandleClearBraveAdsData(
    const base::Value::List& /*args*/) {
  if (auto* ads_service =
          brave_ads::AdsServiceFactory::GetForProfile(profile_)) {
    ads_service->ClearData(/*intentional*/ base::DoNothing());
  }
}

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
