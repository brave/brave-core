/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_UTILITY_REDEEM_UNBLINDED_PAYMENT_TOKENS_REDEEM_UNBLINDED_PAYMENT_TOKENS_URL_REQUEST_BUILDER_H_
#define BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_UTILITY_REDEEM_UNBLINDED_PAYMENT_TOKENS_REDEEM_UNBLINDED_PAYMENT_TOKENS_URL_REQUEST_BUILDER_H_

#include <string>
#include <vector>

#include "base/values.h"
#include "bat/ads/internal/account/wallet/wallet_info.h"
#include "bat/ads/internal/privacy/tokens/unblinded_payment_tokens/unblinded_payment_token_info_aliases.h"
#include "bat/ads/internal/server/url/url_request_builder_interface.h"
#include "bat/ads/public/interfaces/ads.mojom.h"
#include "url/gurl.h"

namespace ads {

namespace privacy {
struct UnblindedPaymentTokenInfo;
}  // namespace privacy

class RedeemUnblindedPaymentTokensUrlRequestBuilder final
    : public UrlRequestBuilderInterface {
 public:
  RedeemUnblindedPaymentTokensUrlRequestBuilder(
      const WalletInfo& wallet,
      const privacy::UnblindedPaymentTokenList& unblinded_payment_tokens,
      const base::Value& user_data);
  ~RedeemUnblindedPaymentTokensUrlRequestBuilder() override;
  RedeemUnblindedPaymentTokensUrlRequestBuilder(
      const RedeemUnblindedPaymentTokensUrlRequestBuilder&) = delete;
  RedeemUnblindedPaymentTokensUrlRequestBuilder& operator=(
      const RedeemUnblindedPaymentTokensUrlRequestBuilder&) = delete;

  mojom::UrlRequestPtr Build() override;

 private:
  GURL BuildUrl() const;

  std::vector<std::string> BuildHeaders() const;

  std::string BuildBody(const std::string& payload) const;

  std::string CreatePayload() const;

  base::Value CreatePaymentRequestDTO(const std::string& payload) const;

  base::Value CreateCredential(
      const privacy::UnblindedPaymentTokenInfo& unblinded_payment_token,
      const std::string& payload) const;

  WalletInfo wallet_;
  privacy::UnblindedPaymentTokenList unblinded_payment_tokens_;
  base::Value user_data_;
};

}  // namespace ads

#endif  // BRAVE_VENDOR_BAT_NATIVE_ADS_SRC_BAT_ADS_INTERNAL_ACCOUNT_UTILITY_REDEEM_UNBLINDED_PAYMENT_TOKENS_REDEEM_UNBLINDED_PAYMENT_TOKENS_URL_REQUEST_BUILDER_H_
