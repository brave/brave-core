/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_COMMON_ENVIRONMENT_CONFIG_H_
#define BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_COMMON_ENVIRONMENT_CONFIG_H_

#include <string>

#include "brave/components/brave_rewards/common/mojom/rewards.mojom.h"
#include "brave/components/brave_rewards/core/rewards_engine_helper.h"
#include "url/gurl.h"

namespace brave_rewards::internal {

// Responsible for providing environment-specific configuration settings.
class EnvironmentConfig : public RewardsEngineHelper {
 public:
  explicit EnvironmentConfig(RewardsEngineImpl& engine);
  ~EnvironmentConfig() override;

  mojom::Environment current_environment() const;

  std::string auto_contribute_sku() const;
  std::string auto_contribute_public_key() const;
  std::string user_funds_public_key() const;

  GURL brave_pcdn_url() const;

  GURL rewards_url() const;
  GURL rewards_api_url() const;
  GURL rewards_grant_url() const;
  GURL rewards_payment_url() const;

  GURL uphold_oauth_url() const;
  GURL uphold_api_url() const;
  std::string uphold_client_id() const;
  std::string uphold_client_secret() const;
  std::string uphold_fee_address() const;
  std::string uphold_sku_destination() const;

  GURL gemini_oauth_url() const;
  GURL gemini_api_url() const;
  std::string gemini_client_id() const;
  std::string gemini_client_secret() const;
  std::string gemini_fee_address() const;
  std::string gemini_sku_destination() const;

  GURL zebpay_oauth_url() const;
  GURL zebpay_api_url() const;
  std::string zebpay_client_id() const;
  std::string zebpay_client_secret() const;

  GURL bitflyer_url() const;
  std::string bitflyer_client_id() const;
  std::string bitflyer_client_secret() const;
  std::string bitflyer_fee_address() const;
};

}  // namespace brave_rewards::internal

#endif  // BRAVE_COMPONENTS_BRAVE_REWARDS_CORE_COMMON_ENVIRONMENT_CONFIG_H_
