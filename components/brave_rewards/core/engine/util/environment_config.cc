/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/engine/util/environment_config.h"

#include <string_view>

#include "base/check.h"
#include "base/strings/strcat.h"
#include "brave/brave_domains/constants.h"
#include "brave/components/brave_rewards/core/engine/buildflags.h"
#include "brave/components/constants/brave_services_key.h"
#include "brave/components/constants/network_constants.h"

namespace brave_rewards::internal {

EnvironmentConfig::EnvironmentConfig(RewardsEngine& engine)
    : RewardsEngineHelper(engine) {
  if (engine.options().is_testing) {
    allow_default_values_for_testing_ = true;
  }
}

EnvironmentConfig::~EnvironmentConfig() = default;

mojom::Environment EnvironmentConfig::current_environment() const {
  return engine().options().environment;
}

GURL EnvironmentConfig::brave_pcdn_url() const {
  switch (current_environment()) {
    case mojom::Environment::kDevelopment:
      return GURL("https://pcdn.brave.software");
    case mojom::Environment::kStaging:
      return GURL("https://pcdn.bravesoftware.com");
    case mojom::Environment::kProduction:
      return GURL("https://pcdn.brave.com");
  }
}

GURL EnvironmentConfig::rewards_url() const {
  switch (current_environment()) {
    case mojom::Environment::kDevelopment:
      return GURL("https://rewards-dev.brave.software");
    case mojom::Environment::kStaging:
      return GURL("https://rewards-stg.bravesoftware.com");
    case mojom::Environment::kProduction:
      return GURL("https://rewards.brave.com");
  }
}

GURL EnvironmentConfig::rewards_api_url() const {
  switch (current_environment()) {
    case mojom::Environment::kDevelopment:
      return GURL("https://api.rewards.brave.software");
    case mojom::Environment::kStaging:
      return GURL("https://api.rewards.bravesoftware.com");
    case mojom::Environment::kProduction:
      return GURL("https://api.rewards.brave.com");
  }
}

GURL EnvironmentConfig::rewards_grant_url() const {
  switch (current_environment()) {
    case mojom::Environment::kDevelopment:
      return URLValue(BUILDFLAG(REWARDS_GRANT_DEV_ENDPOINT));
    case mojom::Environment::kStaging:
      return URLValue(BUILDFLAG(REWARDS_GRANT_STAGING_ENDPOINT));
    case mojom::Environment::kProduction:
      return URLValue(BUILDFLAG(REWARDS_GRANT_PROD_ENDPOINT));
  }
}

GURL EnvironmentConfig::uphold_oauth_url() const {
  return URLValue(current_environment() == mojom::Environment::kProduction
                      ? BUILDFLAG(UPHOLD_PRODUCTION_OAUTH_URL)
                      : BUILDFLAG(UPHOLD_SANDBOX_OAUTH_URL));
}

GURL EnvironmentConfig::uphold_api_url() const {
  return URLValue(current_environment() == mojom::Environment::kProduction
                      ? BUILDFLAG(UPHOLD_PRODUCTION_API_URL)
                      : BUILDFLAG(UPHOLD_SANDBOX_API_URL));
}

std::string EnvironmentConfig::uphold_client_id() const {
  return current_environment() == mojom::Environment::kProduction
             ? BUILDFLAG(UPHOLD_PRODUCTION_CLIENT_ID)
             : BUILDFLAG(UPHOLD_SANDBOX_CLIENT_ID);
}

std::string EnvironmentConfig::uphold_client_secret() const {
  return current_environment() == mojom::Environment::kProduction
             ? BUILDFLAG(UPHOLD_PRODUCTION_CLIENT_SECRET)
             : BUILDFLAG(UPHOLD_SANDBOX_CLIENT_SECRET);
}

std::string EnvironmentConfig::uphold_fee_address() const {
  return current_environment() == mojom::Environment::kProduction
             ? BUILDFLAG(UPHOLD_PRODUCTION_FEE_ADDRESS)
             : BUILDFLAG(UPHOLD_SANDBOX_FEE_ADDRESS);
}

GURL EnvironmentConfig::gemini_oauth_url() const {
  return BuildGate3OAuthURL("gemini");
}

GURL EnvironmentConfig::gemini_api_url() const {
  return URLValue(current_environment() == mojom::Environment::kProduction
                      ? BUILDFLAG(GEMINI_PRODUCTION_API_URL)
                      : BUILDFLAG(GEMINI_SANDBOX_API_URL));
}

std::string EnvironmentConfig::gemini_fee_address() const {
  return current_environment() == mojom::Environment::kProduction
             ? BUILDFLAG(GEMINI_PRODUCTION_FEE_ADDRESS)
             : BUILDFLAG(GEMINI_SANDBOX_FEE_ADDRESS);
}

GURL EnvironmentConfig::zebpay_oauth_url() const {
  return URLValue(current_environment() == mojom::Environment::kProduction
                      ? BUILDFLAG(ZEBPAY_PRODUCTION_OAUTH_URL)
                      : BUILDFLAG(ZEBPAY_SANDBOX_OAUTH_URL));
}

GURL EnvironmentConfig::zebpay_api_url() const {
  return URLValue(current_environment() == mojom::Environment::kProduction
                      ? BUILDFLAG(ZEBPAY_PRODUCTION_API_URL)
                      : BUILDFLAG(ZEBPAY_SANDBOX_API_URL));
}

std::string EnvironmentConfig::zebpay_client_id() const {
  return current_environment() == mojom::Environment::kProduction
             ? BUILDFLAG(ZEBPAY_PRODUCTION_CLIENT_ID)
             : BUILDFLAG(ZEBPAY_SANDBOX_CLIENT_ID);
}

std::string EnvironmentConfig::zebpay_client_secret() const {
  return current_environment() == mojom::Environment::kProduction
             ? BUILDFLAG(ZEBPAY_PRODUCTION_CLIENT_SECRET)
             : BUILDFLAG(ZEBPAY_SANDBOX_CLIENT_SECRET);
}

GURL EnvironmentConfig::bitflyer_url() const {
  return URLValue(current_environment() == mojom::Environment::kProduction
                      ? BUILDFLAG(BITFLYER_PRODUCTION_URL)
                      : BUILDFLAG(BITFLYER_SANDBOX_URL));
}

std::string EnvironmentConfig::bitflyer_client_id() const {
  return current_environment() == mojom::Environment::kProduction
             ? BUILDFLAG(BITFLYER_PRODUCTION_CLIENT_ID)
             : BUILDFLAG(BITFLYER_SANDBOX_CLIENT_ID);
}

std::string EnvironmentConfig::bitflyer_client_secret() const {
  return current_environment() == mojom::Environment::kProduction
             ? BUILDFLAG(BITFLYER_PRODUCTION_CLIENT_SECRET)
             : BUILDFLAG(BITFLYER_SANDBOX_CLIENT_SECRET);
}

std::string EnvironmentConfig::bitflyer_fee_address() const {
  return current_environment() == mojom::Environment::kProduction
             ? BUILDFLAG(BITFLYER_PRODUCTION_FEE_ADDRESS)
             : BUILDFLAG(BITFLYER_SANDBOX_FEE_ADDRESS);
}

std::string EnvironmentConfig::BraveServicesKeyHeader() const {
  return base::StrCat(
      {kBraveServicesKeyHeader, ": ", BUILDFLAG(BRAVE_SERVICES_KEY)});
}

GURL EnvironmentConfig::BuildGate3OAuthURL(std::string_view provider) const {
  std::string environment =
      current_environment() == mojom::Environment::kProduction ? "production"
                                                               : "sandbox";
  return URLValue(base::StrCat({brave_domains::kGate3URL, "/api/oauth/",
                                provider, "/", environment, "/"}));
}

GURL EnvironmentConfig::URLValue(std::string value) const {
  if (value.empty() && allow_default_values_for_testing_) {
    value = "https://example.com";
  }
  GURL url(value);
  DCHECK(url.is_valid());
  return url;
}

}  // namespace brave_rewards::internal
