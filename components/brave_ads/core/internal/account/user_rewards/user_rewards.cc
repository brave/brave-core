/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_rewards/user_rewards.h"

#include <utility>

#include "base/debug/dump_without_crashing.h"
#include "base/functional/bind.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmations_util.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_info.h"
#include "brave/components/brave_ads/core/internal/account/issuers/issuers_util.h"
#include "brave/components/brave_ads/core/internal/account/issuers/url_request/issuers_url_request.h"
#include "brave/components/brave_ads/core/internal/account/user_rewards/user_rewards_util.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_payment_tokens/redeem_payment_tokens.h"
#include "brave/components/brave_ads/core/internal/account/utility/refill_confirmation_tokens/refill_confirmation_tokens.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/common/time/time_formatting_util.h"
#include "brave/components/brave_ads/core/internal/prefs/pref_util.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"
#include "brave/components/brave_ads/core/public/prefs/pref_names.h"

namespace brave_ads {

UserRewards::UserRewards(WalletInfo wallet) : wallet_(std::move(wallet)) {
  CHECK(wallet_.IsValid());

  GetAdsClient()->AddObserver(this);

  issuers_url_request_.SetDelegate(this);
  refill_confirmation_tokens_.SetDelegate(this);
  redeem_payment_tokens_.SetDelegate(this);
}

UserRewards::~UserRewards() {
  GetAdsClient()->RemoveObserver(this);

  delegate_ = nullptr;
}

void UserRewards::FetchIssuers() {
  issuers_url_request_.PeriodicallyFetch();
}

void UserRewards::MaybeRefillConfirmationTokens() {
  refill_confirmation_tokens_.MaybeRefill(wallet_);
}

void UserRewards::MaybeRedeemPaymentTokens() {
  redeem_payment_tokens_.MaybeRedeemAfterDelay(wallet_);
}

///////////////////////////////////////////////////////////////////////////////

void UserRewards::MaybeMigrateVerifiedRewardsUser() {
  if (!ShouldMigrateVerifiedRewardsUser()) {
    return;
  }

  BLOG(1, "Migrate verified rewards user");

  ResetTokens();

  ResetIssuers();
  FetchIssuers();

  SetProfileBooleanPref(prefs::kShouldMigrateVerifiedRewardsUser, false);

  NotifyDidMigrateVerifiedRewardsUser();
}

void UserRewards::NotifyDidMigrateVerifiedRewardsUser() const {
  if (delegate_) {
    delegate_->OnDidMigrateVerifiedRewardsUser();
  }
}

void UserRewards::OnNotifyDidSolveAdaptiveCaptcha() {
  MaybeRefillConfirmationTokens();
}

void UserRewards::OnNotifyPrefDidChange(const std::string& path) {
  if (path == prefs::kShouldMigrateVerifiedRewardsUser) {
    MaybeMigrateVerifiedRewardsUser();
  }
}

void UserRewards::OnDidFetchIssuers(const IssuersInfo& issuers) {
  if (!IsIssuersValid(issuers)) {
    return BLOG(0, "Invalid issuers");
  }

  UpdateIssuers(issuers);

  MaybeRefillConfirmationTokens();
}

void UserRewards::OnDidRedeemPaymentTokens(
    const PaymentTokenList& payment_tokens) {
  transactions_database_table_.Reconcile(
      payment_tokens, base::BindOnce([](const bool success) {
        if (!success) {
          // TODO(https://github.com/brave/brave-browser/issues/32066):
          // Detect potential defects using `DumpWithoutCrashing`.
          SCOPED_CRASH_KEY_STRING64("Issue32066", "failure_reason",
                                    "Failed to reconcile transactions");
          base::debug::DumpWithoutCrashing();

          return BLOG(0, "Failed to reconcile transactions");
        }

        BLOG(3, "Successfully reconciled transactions");
      }));
}

void UserRewards::OnWillRefillConfirmationTokens() {
  BLOG(1, "Refill confirmation tokens");
}

void UserRewards::OnDidRefillConfirmationTokens() {
  BLOG(1, "Successfully refilled confirmation tokens");
}

void UserRewards::OnFailedToRefillConfirmationTokens() {
  BLOG(1, "Failed to refill confirmation tokens");
}

void UserRewards::OnWillRetryRefillingConfirmationTokens(
    const base::Time retry_at) {
  BLOG(1,
       "Retry refilling confirmation tokens " << FriendlyDateAndTime(retry_at));
}

void UserRewards::OnDidRetryRefillingConfirmationTokens() {
  BLOG(1, "Retry refilling confirmation tokens");
}

void UserRewards::OnCaptchaRequiredToRefillConfirmationTokens(
    const std::string& captcha_id) {
  GetAdsClient()->ShowScheduledCaptcha(wallet_.payment_id, captcha_id);
}

}  // namespace brave_ads
