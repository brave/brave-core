/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * you can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/browser/ui/webui/settings/brave_reset_rewards_settings_handler.h"  // NOLINT

#include <utility>

#include "base/bind.h"
#include "base/values.h"
#include "brave/components/brave_rewards/browser/rewards_service.h"
#include "brave/components/brave_rewards/browser/rewards_service_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "content/public/browser/web_ui.h"

namespace settings {

BraveResetRewardsSettingsHandler::BraveResetRewardsSettingsHandler(
    Profile* profile)
    : profile_(profile), callback_weak_ptr_factory_(this) {
}

BraveResetRewardsSettingsHandler::~BraveResetRewardsSettingsHandler() {}

void BraveResetRewardsSettingsHandler::OnJavascriptDisallowed() {
  callback_weak_ptr_factory_.InvalidateWeakPtrs();
}

void BraveResetRewardsSettingsHandler::RegisterMessages() {
  web_ui()->RegisterMessageCallback(
      "performRewardsReset",
      base::BindRepeating(
          &BraveResetRewardsSettingsHandler::HandleResetRewards,
          base::Unretained(this)));
}

void BraveResetRewardsSettingsHandler::HandleResetRewards(
    const base::ListValue* args) {
  AllowJavascript();

  CHECK_EQ(1U, args->GetSize());
  std::string callback_id;
  CHECK(args->GetString(0, &callback_id));
  ResetRewards(callback_id);
}

void BraveResetRewardsSettingsHandler::OnResetRewardsDone(
    std::string callback_id, bool success) {
  ResolveJavascriptCallback(base::Value(callback_id), base::Value(success));
}

void BraveResetRewardsSettingsHandler::ResetRewards(
    const std::string& callback_id) {
  brave_rewards::RewardsService* rewards_service =
      brave_rewards::RewardsServiceFactory::GetForProfile(profile_);
  if (rewards_service) {
    rewards_service->Reset(
        base::BindOnce(&BraveResetRewardsSettingsHandler::OnResetRewardsDone,
          callback_weak_ptr_factory_.GetWeakPtr(),
          callback_id));
  }
}

}  // namespace settings
