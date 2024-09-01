/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_REDEEM_CONFIRMATION_NON_REWARD_REDEEM_NON_REWARD_CONFIRMATION_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_REDEEM_CONFIRMATION_NON_REWARD_REDEEM_NON_REWARD_CONFIRMATION_H_

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/redeem_confirmation_delegate.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

namespace brave_ads {

struct ConfirmationInfo;

class RedeemNonRewardConfirmation final {
 public:
  RedeemNonRewardConfirmation(const RedeemNonRewardConfirmation&) = delete;
  RedeemNonRewardConfirmation& operator=(const RedeemNonRewardConfirmation&) =
      delete;

  RedeemNonRewardConfirmation(RedeemNonRewardConfirmation&&) noexcept;
  RedeemNonRewardConfirmation& operator=(
      RedeemNonRewardConfirmation&&) noexcept;

  ~RedeemNonRewardConfirmation();

  static void CreateAndRedeem(
      base::WeakPtr<RedeemConfirmationDelegate> delegate,
      const ConfirmationInfo& confirmation);

 private:
  explicit RedeemNonRewardConfirmation(
      base::WeakPtr<RedeemConfirmationDelegate> delegate);

  static void Redeem(RedeemNonRewardConfirmation redeem_confirmation,
                     const ConfirmationInfo& confirmation);

  static void CreateConfirmation(
      RedeemNonRewardConfirmation redeem_confirmation,
      const ConfirmationInfo& confirmation);
  static void CreateConfirmationCallback(
      RedeemNonRewardConfirmation redeem_confirmation,
      const ConfirmationInfo& confirmation,
      const mojom::UrlResponseInfo& mojom_url_response);

  void SuccessfullyRedeemedConfirmation(const ConfirmationInfo& confirmation);
  void FailedToRedeemConfirmation(const ConfirmationInfo& confirmation,
                                  bool should_retry);

  void NotifyDidRedeemConfirmation(const ConfirmationInfo& confirmation) const;
  void NotifyFailedToRedeemConfirmation(const ConfirmationInfo& confirmation,
                                        bool should_retry) const;

  base::WeakPtr<RedeemConfirmationDelegate> delegate_ = nullptr;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_REDEEM_CONFIRMATION_NON_REWARD_REDEEM_NON_REWARD_CONFIRMATION_H_
