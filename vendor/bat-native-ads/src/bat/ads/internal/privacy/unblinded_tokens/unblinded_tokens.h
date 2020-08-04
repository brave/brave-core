/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_PRIVACY_UNBLINDED_TOKENS_UNBLINDED_TOKENS_H_
#define BAT_ADS_INTERNAL_PRIVACY_UNBLINDED_TOKENS_UNBLINDED_TOKENS_H_

#include "base/values.h"
#include "bat/ads/internal/privacy/unblinded_tokens/unblinded_token_info.h"

namespace ads {

class AdsImpl;

namespace privacy {

class UnblindedTokens {
 public:
  explicit UnblindedTokens(
      AdsImpl* ads);

  ~UnblindedTokens();

  UnblindedTokenInfo GetToken() const;
  UnblindedTokenList GetAllTokens() const;
  base::Value GetTokensAsList();

  void SetTokens(
      const UnblindedTokenList& unblinded_tokens);
  void SetTokensFromList(
      const base::Value& list);

  void AddTokens(
      const UnblindedTokenList& unblinded_tokens);

  bool RemoveToken(
      const UnblindedTokenInfo& unblinded_token);
  void RemoveAllTokens();

  bool TokenExists(
      const UnblindedTokenInfo& unblinded_token);

  int Count() const;

  bool IsEmpty() const;

 private:
  UnblindedTokenList unblinded_tokens_;

  AdsImpl* ads_;  // NOT OWNED
};

}  // namespace privacy
}  // namespace ads

#endif  // BAT_ADS_INTERNAL_PRIVACY_UNBLINDED_TOKENS_UNBLINDED_TOKENS_H_
