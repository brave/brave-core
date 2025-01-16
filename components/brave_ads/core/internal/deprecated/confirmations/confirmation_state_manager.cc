/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/deprecated/confirmations/confirmation_state_manager.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/json/json_reader.h"
#include "base/json/json_writer.h"
#include "base/ranges/algorithm.h"
#include "base/trace_event/trace_event.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_token_info.h"
#include "brave/components/brave_ads/core/internal/account/tokens/confirmation_tokens/confirmation_tokens_value_util.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_value_util.h"
#include "brave/components/brave_ads/core/internal/ads_client/ads_client_util.h"
#include "brave/components/brave_ads/core/internal/common/challenge_bypass_ristretto/unblinded_token.h"
#include "brave/components/brave_ads/core/internal/common/crypto/crypto_util.h"
#include "brave/components/brave_ads/core/internal/common/logging_util.h"
#include "brave/components/brave_ads/core/internal/global_state/global_state.h"
#include "brave/components/brave_ads/core/public/ads_client/ads_client.h"
#include "brave/components/brave_ads/core/public/ads_constants.h"

namespace brave_ads {

ConfirmationStateManager::ConfirmationStateManager() = default;

ConfirmationStateManager::~ConfirmationStateManager() = default;

// static
ConfirmationStateManager& ConfirmationStateManager::GetInstance() {
  return GlobalState::GetInstance()->GetConfirmationStateManager();
}

void ConfirmationStateManager::LoadState(std::optional<WalletInfo> wallet,
                                         InitializeCallback callback) {
  BLOG(3, "Loading confirmation state");

  wallet_ = std::move(wallet);

  GetAdsClient().Load(
      kConfirmationsJsonFilename,
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
      BLOG(1, "Failed to parse confirmation state: " << *json);

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

  GetAdsClient().Save(kConfirmationsJsonFilename, ToJson(),
                      base::BindOnce([](bool success) {
                        if (!success) {
                          return BLOG(0, "Failed to save confirmation state");
                        }

                        BLOG(9, "Successfully saved confirmation state");
                      }));
}

std::string ConfirmationStateManager::ToJson() {
  TRACE_EVENT(kTraceEventCategory, "ConfirmationStateManager::ToJson");

  std::string json;
  CHECK(base::JSONWriter::Write(
      base::Value::Dict()
          .Set("unblinded_tokens",
               ConfirmationTokensToValue(confirmation_tokens_.GetAll()))
          .Set("unblinded_payment_tokens",
               PaymentTokensToValue(payment_tokens_.GetAllTokens())),
      &json));
  return json;
}

bool ConfirmationStateManager::FromJson(const std::string& json) {
  TRACE_EVENT(kTraceEventCategory, "ConfirmationStateManager::FromJson", "json",
              json.size());

  const std::optional<base::Value::Dict> dict =
      base::JSONReader::ReadDict(json);
  confirmation_tokens_.RemoveAll();
  payment_tokens_.RemoveAllTokens();

  if (!dict) {
    BLOG(0, "Malformed confirmation JSON state");
    return false;
  }

  ParseConfirmationTokensFromDictionary(*dict);

  ParsePaymentTokensFromDictionary(*dict);

  return true;
}

///////////////////////////////////////////////////////////////////////////////

void ConfirmationStateManager::ParseConfirmationTokensFromDictionary(
    const base::Value::Dict& dict) {
  const auto* const list = dict.FindList("unblinded_tokens");
  if (!list) {
    return;
  }

  ConfirmationTokenList filtered_confirmation_tokens =
      ConfirmationTokensFromValue(*list);

  if (wallet_ && !filtered_confirmation_tokens.empty()) {
    const std::string public_key_base64 = wallet_->public_key_base64;

    filtered_confirmation_tokens.erase(
        base::ranges::remove_if(
            filtered_confirmation_tokens,
            [&public_key_base64](
                const ConfirmationTokenInfo& confirmation_token) {
              const std::optional<std::string> unblinded_token_base64 =
                  confirmation_token.unblinded_token.EncodeBase64();
              return !unblinded_token_base64 ||
                     !crypto::Verify(*unblinded_token_base64, public_key_base64,
                                     confirmation_token.signature_base64);
            }),
        filtered_confirmation_tokens.cend());
  }

  confirmation_tokens_.Set(filtered_confirmation_tokens);
}

void ConfirmationStateManager::ParsePaymentTokensFromDictionary(
    const base::Value::Dict& dict) {
  if (const auto* const list = dict.FindList("unblinded_payment_tokens")) {
    payment_tokens_.SetTokens(PaymentTokensFromValue(*list));
  }
}

}  // namespace brave_ads
