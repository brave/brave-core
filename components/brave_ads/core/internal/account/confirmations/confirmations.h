/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATIONS_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATIONS_H_

#include "base/memory/raw_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/values.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/confirmations_delegate.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/confirmation_queue.h"
#include "brave/components/brave_ads/core/internal/account/confirmations/queue/confirmation_queue_delegate.h"
#include "brave/components/brave_ads/core/internal/account/utility/redeem_confirmation/redeem_confirmation_delegate.h"

namespace brave_ads {

struct TransactionInfo;

class Confirmations final : public ConfirmationQueueDelegate,
                            public RedeemConfirmationDelegate {
 public:
  Confirmations();

  Confirmations(const Confirmations&) = delete;
  Confirmations& operator=(const Confirmations&) = delete;

  Confirmations(Confirmations&&) noexcept = delete;
  Confirmations& operator=(Confirmations&&) noexcept = delete;

  ~Confirmations() override;

  void SetDelegate(ConfirmationDelegate* delegate) {
    CHECK_EQ(delegate_, nullptr);
    delegate_ = delegate;
  }

  void Confirm(const TransactionInfo& transaction, base::Value::Dict user_data);

 private:
  void NotifyDidConfirm(const ConfirmationInfo& confirmation) const;
  void NotifyFailedToConfirm(const ConfirmationInfo& confirmation) const;

  // ConfirmationQueueDelegate:
  void OnDidAddConfirmationToQueue(
      const ConfirmationInfo& confirmation) override;
  void OnWillProcessConfirmationQueue(const ConfirmationInfo& confirmation,
                                      base::Time process_at) override;
  void OnDidProcessConfirmationQueue(
      const ConfirmationInfo& confirmation) override;
  void OnFailedToProcessConfirmationQueue(
      const ConfirmationInfo& confirmation) override;
  void OnDidExhaustConfirmationQueue() override;

  raw_ptr<ConfirmationDelegate> delegate_ = nullptr;

  ConfirmationQueue confirmation_queue_;

  base::WeakPtrFactory<Confirmations> weak_factory_{this};
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_CONFIRMATIONS_CONFIRMATIONS_H_
