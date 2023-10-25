/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PROMOTION_PROMOTION_SERVER_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PROMOTION_PROMOTION_SERVER_H_

#include "brave/components/brave_rewards/core/endpoint/promotion/get_available/get_available.h"
#include "brave/components/brave_rewards/core/endpoint/promotion/get_captcha/get_captcha.h"
#include "brave/components/brave_rewards/core/endpoint/promotion/get_signed_creds/get_signed_creds.h"
#include "brave/components/brave_rewards/core/endpoint/promotion/post_bat_loss/post_bat_loss.h"
#include "brave/components/brave_rewards/core/endpoint/promotion/post_captcha/post_captcha.h"
#include "brave/components/brave_rewards/core/endpoint/promotion/post_clobbered_claims/post_clobbered_claims.h"
#include "brave/components/brave_rewards/core/endpoint/promotion/post_creds/post_creds.h"
#include "brave/components/brave_rewards/core/endpoint/promotion/post_devicecheck/post_devicecheck.h"
#include "brave/components/brave_rewards/core/endpoint/promotion/post_safetynet/post_safetynet.h"
#include "brave/components/brave_rewards/core/endpoint/promotion/post_suggestions/post_suggestions.h"
#include "brave/components/brave_rewards/core/endpoint/promotion/post_suggestions_claim/post_suggestions_claim.h"
#include "brave/components/brave_rewards/core/endpoint/promotion/put_captcha/put_captcha.h"
#include "brave/components/brave_rewards/core/endpoint/promotion/put_devicecheck/put_devicecheck.h"
#include "brave/components/brave_rewards/core/endpoint/promotion/put_safetynet/put_safetynet.h"

namespace brave_rewards::internal {
class RewardsEngineImpl;

namespace endpoint {

class PromotionServer {
 public:
  explicit PromotionServer(RewardsEngineImpl& engine);
  ~PromotionServer();

  promotion::GetAvailable& get_available() { return get_available_; }

  promotion::PostCreds& post_creds() { return post_creds_; }

  promotion::GetSignedCreds& get_signed_creds() { return get_signed_creds_; }

  promotion::PostClobberedClaims& post_clobbered_claims() {
    return post_clobbered_claims_;
  }

  promotion::PostBatLoss& post_bat_loss() { return post_bat_loss_; }

  promotion::PostCaptcha& post_captcha() { return post_captcha_; }

  promotion::GetCaptcha& get_captcha() { return get_captcha_; }

  promotion::PutCaptcha& put_captcha() { return put_captcha_; }

  promotion::PostSafetynet& post_safetynet() { return post_safetynet_; }

  promotion::PutSafetynet& put_safetynet() { return put_safetynet_; }

  promotion::PostDevicecheck& post_devicecheck() { return post_devicecheck_; }

  promotion::PutDevicecheck& put_devicecheck() { return put_devicecheck_; }

  promotion::PostSuggestions& post_suggestions() { return post_suggestions_; }

  promotion::PostSuggestionsClaim& post_suggestions_claim() {
    return post_suggestions_claim_;
  }

 private:
  promotion::GetAvailable get_available_;
  promotion::PostCreds post_creds_;
  promotion::GetSignedCreds get_signed_creds_;
  promotion::PostClobberedClaims post_clobbered_claims_;
  promotion::PostBatLoss post_bat_loss_;
  promotion::PostCaptcha post_captcha_;
  promotion::GetCaptcha get_captcha_;
  promotion::PutCaptcha put_captcha_;
  promotion::PostSafetynet post_safetynet_;
  promotion::PutSafetynet put_safetynet_;
  promotion::PostDevicecheck post_devicecheck_;
  promotion::PutDevicecheck put_devicecheck_;
  promotion::PostSuggestions post_suggestions_;
  promotion::PostSuggestionsClaim post_suggestions_claim_;
};

}  // namespace endpoint
}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_ENDPOINT_PROMOTION_PROMOTION_SERVER_H_
