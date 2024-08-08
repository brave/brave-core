/* Copyright (c) 2024 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_CORE_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_CORE_H_

#include <memory>

#include "brave/components/brave_ads/core/internal/account/account.h"
#include "brave/components/brave_ads/core/internal/ad_units/ad_handler.h"
#include "brave/components/brave_ads/core/internal/reminders/reminders.h"
#include "brave/components/brave_ads/core/internal/studies/studies.h"
#include "brave/components/brave_ads/core/internal/user_attention/user_idle_detection/user_idle_detection.h"
#include "brave/components/brave_ads/core/internal/user_engagement/reactions/reactions.h"

namespace brave_ads {

class TokenGeneratorInterface;

class AdsCore final {
 public:
  explicit AdsCore(std::unique_ptr<TokenGeneratorInterface> token_generator);

  AdsCore(const AdsCore& other) = delete;
  AdsCore& operator=(const AdsCore& other) = delete;

  AdsCore(AdsCore&& other) noexcept = delete;
  AdsCore& operator=(AdsCore&& other) noexcept = delete;

  ~AdsCore();

  // Provides methods for generating tokens for refilling and redeeming
  // confirmation tokens.
  const TokenGeneratorInterface* GetTokenGenerator() const;

  // Provides methods for managing user rewards, wallets, account statements,
  // and deposits.
  Account& GetAccount();

  // Provides methods for serving and triggering events for various types of
  // ads, including inline content ads, new tab page ads, notification ads,
  // promoted content ads, and search result ads.
  AdHandler& GetAdHandler();

  // Provides methods for engaging with ads, such as liking, disliking, marking
  // as inappropriate, and saving ads.
  Reactions& GetReactions();

 private:
  const std::unique_ptr<TokenGeneratorInterface> token_generator_;

  Account account_;

  AdHandler ad_handler_;

  Reactions reactions_;

  // Handles the delivery of helpful reminders to users on how to interact with
  // Brave Ads.
  Reminders reminders_;

  // Handles notifying observers when the user becomes active or idle, and
  // whether the screen is locked.
  UserIdleDetection user_idle_detection_;

  // Handles user studies, a set of experiments conducted on the client.
  Studies studies_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ADS_CORE_H_
