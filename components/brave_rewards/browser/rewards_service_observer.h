// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef BRAVE_BROWSER_PAYMENTS_PAYMENTS_SERVICE_OBSERVER_H_
#define BRAVE_BROWSER_PAYMENTS_PAYMENTS_SERVICE_OBSERVER_H_

#include "brave/components/brave_rewards/browser/wallet_properties.h"
#include "brave/components/brave_rewards/browser/promotion.h"

namespace brave_rewards {

class RewardsService;

class RewardsServiceObserver {
 public:
  virtual ~RewardsServiceObserver() {}

  virtual void OnWalletInitialized(RewardsService* payment_service,
                               int error_code) {};
  virtual void OnWalletProperties(
      RewardsService* payment_service,
      int error_code,
      std::unique_ptr<brave_rewards::WalletProperties> properties) {};
  virtual void OnPromotion(RewardsService* payment_service,
                           unsigned int error_code,
                           brave_rewards::Promotion properties) {};
  virtual void OnPromotionCaptcha(RewardsService* payment_service,
                                  std::string image) {};
  virtual void OnRecoverWallet(RewardsService* payment_service,
                               unsigned int result,
                               double balance,
                               std::vector<brave_rewards::Grant> grants) {};
  virtual void OnPromotionFinish(RewardsService* payment_service,
                                 unsigned int result,
                                 unsigned int statusCode,
                                 uint64_t expirationDate) {};
};

}  // namespace brave_rewards

#endif  // BRAVE_BROWSER_PAYMENTS_PAYMENTS_SERVICE_OBSERVER_H_
