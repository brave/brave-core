/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_REDEEM_PAYMENT_TOKENS_URL_REQUEST_BUILDERS_REDEEM_PAYMENT_TOKENS_URL_REQUEST_BUILDER_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_REDEEM_PAYMENT_TOKENS_URL_REQUEST_BUILDERS_REDEEM_PAYMENT_TOKENS_URL_REQUEST_BUILDER_H_

#include <string>

#include "base/values.h"
#include "brave/components/brave_ads/core/internal/account/tokens/payment_tokens/payment_token_info.h"
#include "brave/components/brave_ads/core/internal/account/wallet/wallet_info.h"
#include "brave/components/brave_ads/core/internal/common/url/request_builder/url_request_builder_interface.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom-forward.h"

class GURL;

namespace brave_ads {

class RedeemPaymentTokensUrlRequestBuilder final
    : public UrlRequestBuilderInterface {
 public:
  RedeemPaymentTokensUrlRequestBuilder(WalletInfo wallet,
                                       PaymentTokenList payment_tokens,
                                       base::Value::Dict user_data);

  RedeemPaymentTokensUrlRequestBuilder(
      const RedeemPaymentTokensUrlRequestBuilder&) = delete;
  RedeemPaymentTokensUrlRequestBuilder& operator=(
      const RedeemPaymentTokensUrlRequestBuilder&) = delete;

  ~RedeemPaymentTokensUrlRequestBuilder() override;

  mojom::UrlRequestInfoPtr Build() override;

 private:
  GURL BuildUrl() const;

  std::string BuildBody(const std::string& payload);

  std::string BuildPayload() const;

  base::Value::List BuildPaymentRequestDTO(const std::string& payload) const;

  WalletInfo wallet_;
  PaymentTokenList payment_tokens_;
  base::Value::Dict user_data_;
};

}  // namespace brave_ads

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_ACCOUNT_UTILITY_REDEEM_PAYMENT_TOKENS_URL_REQUEST_BUILDERS_REDEEM_PAYMENT_TOKENS_URL_REQUEST_BUILDER_H_
