/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/deprecated/confirmations/confirmation_state_manager.h"

#include <utility>

#include "base/check.h"
#include "base/debug/dump_without_crashing.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/ranges/algorithm.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_token_info.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_value_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_value_util.h"
#include "brave/components/brave_ads/core/internal/client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/unblinded_token.h"
#include "brave/components/brave_ads/core/internal/common/crypto/crypto_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/deprecated/confirmations/confirmation_state_manager_constants.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"

namespace brave_ads {

ConfirmationStateManager::ConfirmationStateManager() = default;

ConfirmationStateManager::~ConfirmationStateManager() = default;

// static
ConfirmationStateManager& ConfirmationStateManager::GetInstance() {
  return GlobalState::GetInstance()->GetConfirmationStateManager();
}

void ConfirmationStateManager::LoadState(
    const std::optional<WalletInfo>& wallet,
    InitializeCallback callback) {
  BLOG(3, "Loading confirmation state");

  wallet_ = wallet;

  Load(kConfirmationStateFilename,
       base::BindOnce(&ConfirmationStateManager::LoadCallback,
                      weak_factory_.GetWeakPtr(), std::move(callback)));
}

void ConfirmationStateManager::LoadCallback(
    InitializeCallback callback,
    const std::optional<std::string>& json) {
  if (!json) {
    BLOG(3, "Confirmation state does not exist, creating default state");

    is_initialized_ = true;

    SaveState();
  } else {
    if (!FromJson(*json)) {
      BLOG(0, "Failed to load confirmation state");
      BLOG(3, "Failed to parse confirmation state: " << *json);

      return std::move(callback).Run(/*success=*/false);
    }

    BLOG(3, "Successfully loaded confirmation state");

    is_initialized_ = true;
  }

  std::move(callback).Run(/*success=*/true);
}

void ConfirmationStateManager::SaveState() {
  if (!is_initialized_) {
    return;
  }

  BLOG(9, "Saving confirmation state");

  Save(kConfirmationStateFilename, ToJson(),
       base::BindOnce([](const bool success) {
         if (!success) {
           return BLOG(0, "Failed to save confirmation state");
         }

         BLOG(9, "Successfully saved confirmation state");
       }));
}

std::string ConfirmationStateManager::ToJson() {
  base::Value::Dict dict;

  // Unblinded tokens
  dict.Set("unblinded_tokens",
           ConfirmationTokensToValue(confirmation_tokens_.GetAll()));

  // Payment tokens
  dict.Set("unblinded_payment_tokens",
           PaymentTokensToValue(payment_tokens_.GetAllTokens()));

  // Write to JSON
  std::string json;
  CHECK(base::JSONWriter::Write(dict, &json));
  return json;
}

bool ConfirmationStateManager::FromJson(const std::string& json) {
  const std::optional<base::Value::Dict> dict =
      base::JSONReader::ReadDict(json);
  confirmation_tokens_.RemoveAll();
  payment_tokens_.RemoveAllTokens();

  if (!dict) {
    // TODO(https://github.com/brave/brave-browser/issues/32066):
    // Remove migration failure dumps.
    base::debug::DumpWithoutCrashing();

    return false;
  }

  if (!ParseConfirmationTokensFromDictionary(*dict)) {
    // TODO(https://github.com/brave/brave-browser/issues/32066):
    // Remove migration failure dumps.
    base::debug::DumpWithoutCrashing();

    BLOG(1, "Failed to parse confirmation tokens");
  }

  if (!ParsePaymentTokensFromDictionary(*dict)) {
    // TODO(https://github.com/brave/brave-browser/issues/32066):
    // Remove migration failure dumps.
    base::debug::DumpWithoutCrashing();

    BLOG(1, "Failed to parse payment tokens");
  }

  return true;
}

///////////////////////////////////////////////////////////////////////////////

bool ConfirmationStateManager::ParseConfirmationTokensFromDictionary(
    const base::Value::Dict& dict) {
  const auto* const list = dict.FindList("unblinded_tokens");
  if (!list) {
    return false;
  }

  ConfirmationTokenList filtered_confirmation_tokens =
      ConfirmationTokensFromValue(*list);

  if (wallet_ && !filtered_confirmation_tokens.empty()) {
    const std::string public_key = wallet_->public_key;

    filtered_confirmation_tokens.erase(
        base::ranges::remove_if(
            filtered_confirmation_tokens,
            [&public_key](const ConfirmationTokenInfo& confirmation_token) {
              const std::optional<std::string> unblinded_token_base64 =
                  confirmation_token.unblinded_token.EncodeBase64();
              return !unblinded_token_base64 ||
                     !crypto::Verify(*unblinded_token_base64, public_key,
                                     confirmation_token.signature);
            }),
        filtered_confirmation_tokens.cend());
  }

  confirmation_tokens_.Set(filtered_confirmation_tokens);

  return true;
}

bool ConfirmationStateManager::ParsePaymentTokensFromDictionary(
    const base::Value::Dict& dict) {
  const auto* const list = dict.FindList("unblinded_payment_tokens");
  if (!list) {
    return false;
  }

  payment_tokens_.SetTokens(PaymentTokensFromValue(*list));

  return true;
}

}  // namespace brave_ads
