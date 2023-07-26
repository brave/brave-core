/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_REDEEM_CONFIRMATION_REWARD_REDEEM_REWARD_CONFIRMATION_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_REDEEM_CONFIRMATION_REWARD_REDEEM_REWARD_CONFIRMATION_H_

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/internal/account/tokens/redeem_confirmation/redeem_confirmation_delegate.h"

namespace brave_ads {

struct ConfirmationInfo;

class RedeemRewardConfirmation final {
 public:
  RedeemRewardConfirmation(const RedeemRewardConfirmation&) = delete;
  RedeemRewardConfirmation& operator=(const RedeemRewardConfirmation&) = delete;

  RedeemRewardConfirmation(RedeemRewardConfirmation&&) noexcept;
  RedeemRewardConfirmation& operator=(RedeemRewardConfirmation&&) noexcept;

  ~RedeemRewardConfirmation();

  static void CreateAndRedeem(
      base::WeakPtr<RedeemConfirmationDelegate> delegate,
      const ConfirmationInfo& confirmation);

 private:
  explicit RedeemRewardConfirmation(
      base::WeakPtr<RedeemConfirmationDelegate> delegate);

  static void Redeem(RedeemRewardConfirmation redeem_confirmation,
                     const ConfirmationInfo& confirmation);

  static void CreateConfirmation(RedeemRewardConfirmation redeem_confirmation,
                                 const ConfirmationInfo& confirmation);
  static void CreateConfirmationCallback(
      RedeemRewardConfirmation redeem_confirmation,
      const ConfirmationInfo& confirmation,
      const mojom::UrlResponseInfo& url_response);

  static void FetchPaymentToken(RedeemRewardConfirmation redeem_confirmation,
                                const ConfirmationInfo& confirmation);
  static void FetchPaymentTokenCallback(
      RedeemRewardConfirmation redeem_confirmation,
      const ConfirmationInfo& confirmation,
      const mojom::UrlResponseInfo& url_response);

  void SuccessfullyRedeemedConfirmation(const ConfirmationInfo& confirmation);
  void FailedToRedeemConfirmation(const ConfirmationInfo& confirmation,
                                  bool should_retry);

  base::WeakPtr<RedeemConfirmationDelegate> delegate_ = nullptr;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_TOKENS_REDEEM_CONFIRMATION_REWARD_REDEEM_REWARD_CONFIRMATION_H_
