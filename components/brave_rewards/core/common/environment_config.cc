/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_rewards/core/common/environment_config.h"

#include "brave/components/brave_rewards/core/buildflags.h"

namespace brave_rewards::internal {

EnvironmentConfig::EnvironmentConfig(RewardsEngineImpl& engine)
    : RewardsEngineHelper(engine) {}

EnvironmentConfig::~EnvironmentConfig() = default;

mojom::Environment EnvironmentConfig::current_environment() const {
  return _environment;
}

std::string EnvironmentConfig::auto_contribute_sku() const {
  switch (_environment) {
    case mojom::Environment::kDevelopment:
      return "AgEJYnJhdmUuY29tAiNicmF2ZSB1c2VyLXdhbGxldC12b3RlIHNrdSB0b2tlbiB2M"
             "QACFHNrdT11c2VyLXdhbGxldC12b3RlAAIKcHJpY2U9MC4yNQACDGN1cnJlbmN5PU"
             "JBVAACDGRlc2NyaXB0aW9uPQACGmNyZWRlbnRpYWxfdHlwZT1zaW5nbGUtdXNlAAA"
             "GINiB9dUmpqLyeSEdZ23E4dPXwIBOUNJCFN9d5toIME2M";
    case mojom::Environment::kStaging:
      return "AgEJYnJhdmUuY29tAiNicmF2ZSB1c2VyLXdhbGxldC12b3RlIHNrdSB0b2tlbiB2M"
             "QACFHNrdT11c2VyLXdhbGxldC12b3RlAAIKcHJpY2U9MC4yNQACDGN1cnJlbmN5PU"
             "JBVAACDGRlc2NyaXB0aW9uPQACGmNyZWRlbnRpYWxfdHlwZT1zaW5nbGUtdXNlAAA"
             "GIOH4Li+rduCtFOfV8Lfa2o8h4SQjN5CuIwxmeQFjOk4W";
    case mojom::Environment::kProduction:
      return "AgEJYnJhdmUuY29tAiNicmF2ZSB1c2VyLXdhbGxldC12b3RlIHNrdSB0b2tlbiB2M"
             "QACFHNrdT11c2VyLXdhbGxldC12b3RlAAIKcHJpY2U9MC4yNQACDGN1cnJlbmN5PU"
             "JBVAACDGRlc2NyaXB0aW9uPQACGmNyZWRlbnRpYWxfdHlwZT1zaW5nbGUtdXNlAAA"
             "GIOaNAUCBMKm0IaLqxefhvxOtAKB0OfoiPn0NPVfI602J";
  }
}

std::string EnvironmentConfig::auto_contribute_public_key() const {
  switch (_environment) {
    case mojom::Environment::kDevelopment:
      return "RhfxGp4pT0Kqe2zx4+q+L6lwC3G9v3fIj1L+PbINNzw=";
    case mojom::Environment::kStaging:
      return "mMMWZrWPlO5b9IB8vF5kUJW4f7ULH1wuEop3NOYqNW0=";
    case mojom::Environment::kProduction:
      return "yr4w9Y0XZQISBOToATNEl5ADspDUgm7cBSOhfYgPWx4=";
  }
}

std::string EnvironmentConfig::user_funds_public_key() const {
  switch (_environment) {
    case mojom::Environment::kDevelopment:
      return "nsSoWgGMJpIiCGVdYrne03ldQ4zqZOMERVD5eSPhhxc=";
    case mojom::Environment::kStaging:
      return "CMezK92X5wmYHVYpr22QhNsTTq6trA/N9Alw+4cKyUY=";
    case mojom::Environment::kProduction:
      return "PGLvfpIn8QXuQJEtv2ViQSWw2PppkhexKr1mlvwCpnM=";
  }
}

GURL EnvironmentConfig::brave_pcdn_url() const {
  switch (_environment) {
    case mojom::Environment::kDevelopment:
      return GURL("https://pcdn.brave.software");
    case mojom::Environment::kStaging:
      return GURL("https://pcdn.bravesoftware.com");
    case mojom::Environment::kProduction:
      return GURL("https://pcdn.brave.com");
  }
}

GURL EnvironmentConfig::rewards_url() const {
  switch (_environment) {
    case mojom::Environment::kDevelopment:
      return GURL("https://rewards-dev.brave.software");
    case mojom::Environment::kStaging:
      return GURL("https://rewards-stg.bravesoftware.com");
    case mojom::Environment::kProduction:
      return GURL("https://rewards.brave.com");
  }
}

GURL EnvironmentConfig::rewards_api_url() const {
  switch (_environment) {
    case mojom::Environment::kDevelopment:
      return GURL("https://api.rewards.brave.software");
    case mojom::Environment::kStaging:
      return GURL("https://api.rewards.bravesoftware.com");
    case mojom::Environment::kProduction:
      return GURL("https://api.rewards.brave.com");
  }
}

GURL EnvironmentConfig::rewards_grant_url() const {
  switch (_environment) {
    case mojom::Environment::kDevelopment:
      return GURL(BUILDFLAG(REWARDS_GRANT_DEV_ENDPOINT));
    case mojom::Environment::kStaging:
      return GURL(BUILDFLAG(REWARDS_GRANT_STAGING_ENDPOINT));
    case mojom::Environment::kProduction:
      return GURL(BUILDFLAG(REWARDS_GRANT_PROD_ENDPOINT));
  }
}

GURL EnvironmentConfig::rewards_payment_url() const {
  switch (_environment) {
    case mojom::Environment::kDevelopment:
      return GURL("https://payment.rewards.brave.software");
    case mojom::Environment::kStaging:
      return GURL("https://payment.rewards.bravesoftware.com");
    case mojom::Environment::kProduction:
      return GURL("https://payment.rewards.brave.com");
  }
}

GURL EnvironmentConfig::uphold_oauth_url() const {
  return _environment == mojom::Environment::kProduction
             ? GURL(BUILDFLAG(UPHOLD_PRODUCTION_OAUTH_URL))
             : GURL(BUILDFLAG(UPHOLD_SANDBOX_OAUTH_URL));
}

GURL EnvironmentConfig::uphold_api_url() const {
  return _environment == mojom::Environment::kProduction
             ? GURL(BUILDFLAG(UPHOLD_PRODUCTION_API_URL))
             : GURL(BUILDFLAG(UPHOLD_SANDBOX_API_URL));
}

std::string EnvironmentConfig::uphold_client_id() const {
  return _environment == mojom::Environment::kProduction
             ? BUILDFLAG(UPHOLD_PRODUCTION_CLIENT_ID)
             : BUILDFLAG(UPHOLD_SANDBOX_CLIENT_ID);
}

std::string EnvironmentConfig::uphold_client_secret() const {
  return _environment == mojom::Environment::kProduction
             ? BUILDFLAG(UPHOLD_PRODUCTION_CLIENT_SECRET)
             : BUILDFLAG(UPHOLD_SANDBOX_CLIENT_SECRET);
}

std::string EnvironmentConfig::uphold_fee_address() const {
  return _environment == mojom::Environment::kProduction
             ? BUILDFLAG(UPHOLD_PRODUCTION_FEE_ADDRESS)
             : BUILDFLAG(UPHOLD_SANDBOX_FEE_ADDRESS);
}

std::string EnvironmentConfig::uphold_sku_destination() const {
  switch (_environment) {
    case mojom::Environment::kDevelopment:
      return "9094c3f2-b3ae-438f-bd59-92aaad92de5c";
    case mojom::Environment::kStaging:
      return "6654ecb0-6079-4f6c-ba58-791cc890a561";
    case mojom::Environment::kProduction:
      return "5d4be2ad-1c65-4802-bea1-e0f3a3a487cb";
  }
}

GURL EnvironmentConfig::gemini_oauth_url() const {
  return _environment == mojom::Environment::kProduction
             ? GURL(BUILDFLAG(GEMINI_PRODUCTION_OAUTH_URL))
             : GURL(BUILDFLAG(GEMINI_SANDBOX_OAUTH_URL));
}

GURL EnvironmentConfig::gemini_api_url() const {
  return _environment == mojom::Environment::kProduction
             ? GURL(BUILDFLAG(GEMINI_PRODUCTION_API_URL))
             : GURL(BUILDFLAG(GEMINI_SANDBOX_API_URL));
}

std::string EnvironmentConfig::gemini_client_id() const {
  return _environment == mojom::Environment::kProduction
             ? BUILDFLAG(GEMINI_PRODUCTION_CLIENT_ID)
             : BUILDFLAG(GEMINI_SANDBOX_CLIENT_ID);
}

std::string EnvironmentConfig::gemini_client_secret() const {
  return _environment == mojom::Environment::kProduction
             ? BUILDFLAG(GEMINI_PRODUCTION_CLIENT_SECRET)
             : BUILDFLAG(GEMINI_SANDBOX_CLIENT_SECRET);
}

std::string EnvironmentConfig::gemini_fee_address() const {
  return _environment == mojom::Environment::kProduction
             ? BUILDFLAG(GEMINI_PRODUCTION_FEE_ADDRESS)
             : BUILDFLAG(GEMINI_SANDBOX_FEE_ADDRESS);
}

std::string EnvironmentConfig::gemini_sku_destination() const {
  switch (_environment) {
    case mojom::Environment::kDevelopment:
      return "60e5e863-8c3d-4341-8b54-23e2695a490c";
    case mojom::Environment::kStaging:
      return "622b9018-f26a-44bf-9a45-3bf3bf3c95e9";
    case mojom::Environment::kProduction:
      return "6116adaf-92e6-42fa-bee8-6f749b8eb44e";
  }
}

GURL EnvironmentConfig::zebpay_oauth_url() const {
  return _environment == mojom::Environment::kProduction
             ? GURL(BUILDFLAG(ZEBPAY_PRODUCTION_OAUTH_URL))
             : GURL(BUILDFLAG(ZEBPAY_SANDBOX_OAUTH_URL));
}

GURL EnvironmentConfig::zebpay_api_url() const {
  return _environment == mojom::Environment::kProduction
             ? GURL(BUILDFLAG(ZEBPAY_PRODUCTION_API_URL))
             : GURL(BUILDFLAG(ZEBPAY_SANDBOX_API_URL));
}

std::string EnvironmentConfig::zebpay_client_id() const {
  return _environment == mojom::Environment::kProduction
             ? BUILDFLAG(ZEBPAY_PRODUCTION_CLIENT_ID)
             : BUILDFLAG(ZEBPAY_SANDBOX_CLIENT_ID);
}

std::string EnvironmentConfig::zebpay_client_secret() const {
  return _environment == mojom::Environment::kProduction
             ? BUILDFLAG(ZEBPAY_PRODUCTION_CLIENT_SECRET)
             : BUILDFLAG(ZEBPAY_SANDBOX_CLIENT_SECRET);
}

GURL EnvironmentConfig::bitflyer_url() const {
  return _environment == mojom::Environment::kProduction
             ? GURL(BUILDFLAG(BITFLYER_PRODUCTION_URL))
             : GURL(BUILDFLAG(BITFLYER_SANDBOX_URL));
}

std::string EnvironmentConfig::bitflyer_client_id() const {
  return _environment == mojom::Environment::kProduction
             ? BUILDFLAG(BITFLYER_PRODUCTION_CLIENT_ID)
             : BUILDFLAG(BITFLYER_SANDBOX_CLIENT_ID);
}

std::string EnvironmentConfig::bitflyer_client_secret() const {
  return _environment == mojom::Environment::kProduction
             ? BUILDFLAG(BITFLYER_PRODUCTION_CLIENT_SECRET)
             : BUILDFLAG(BITFLYER_SANDBOX_CLIENT_SECRET);
}

std::string EnvironmentConfig::bitflyer_fee_address() const {
  return _environment == mojom::Environment::kProduction
             ? BUILDFLAG(BITFLYER_PRODUCTION_FEE_ADDRESS)
             : BUILDFLAG(BITFLYER_SANDBOX_FEE_ADDRESS);
}

}  // namespace brave_rewards::internal
