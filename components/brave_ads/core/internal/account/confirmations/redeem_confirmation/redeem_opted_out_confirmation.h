/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_REDEEM_CONFIRMATION_REDEEM_OPTED_OUT_CONFIRMATION_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_REDEEM_CONFIRMATION_REDEEM_OPTED_OUT_CONFIRMATION_H_

#include "base/memory/weak_ptr.h"
#include "brave/components/brave_ads/common/interfaces/brave_ads.mojom-forward.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/redeem_confirmation/redeem_confirmation_delegate.h"

namespace brave_ads {

struct ConfirmationInfo;

class RedeemOptedOutConfirmation final {
 public:
  RedeemOptedOutConfirmation(const RedeemOptedOutConfirmation&) = delete;
  RedeemOptedOutConfirmation& operator=(const RedeemOptedOutConfirmation&) =
      delete;

  RedeemOptedOutConfirmation(RedeemOptedOutConfirmation&&) noexcept;
  RedeemOptedOutConfirmation& operator=(RedeemOptedOutConfirmation&&) noexcept;

  ~RedeemOptedOutConfirmation();

  static void CreateAndRedeem(
      base::WeakPtr<RedeemConfirmationDelegate> delegate,
      const ConfirmationInfo& confirmation);

 private:
  explicit RedeemOptedOutConfirmation(
      base::WeakPtr<RedeemConfirmationDelegate> delegate);

  static void Redeem(RedeemOptedOutConfirmation redeem_confirmation,
                     const ConfirmationInfo& confirmation);

  static void CreateConfirmation(RedeemOptedOutConfirmation redeem_confirmation,
                                 const ConfirmationInfo& confirmation);
  static void CreateConfirmationCallback(
      RedeemOptedOutConfirmation redeem_confirmation,
      const ConfirmationInfo& confirmation,
      const mojom::UrlResponseInfo& url_response);

  void SuccessfullyRedeemedConfirmation(const ConfirmationInfo& confirmation);
  void FailedToRedeemConfirmation(const ConfirmationInfo& confirmation,
                                  bool should_retry,
                                  bool should_backoff);

  base::WeakPtr<RedeemConfirmationDelegate> delegate_ = nullptr;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_REDEEM_CONFIRMATION_REDEEM_OPTED_OUT_CONFIRMATION_H_
