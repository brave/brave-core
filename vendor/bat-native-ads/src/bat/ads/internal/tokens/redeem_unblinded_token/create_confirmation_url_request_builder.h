/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BAT_ADS_INTERNAL_TOKENS_REDEEM_UNBLINDED_TOKEN_CREATE_CONFIRMATION_URL_REQUEST_BUILDER_H_  // NOLINT
#define BAT_ADS_INTERNAL_TOKENS_REDEEM_UNBLINDED_TOKEN_CREATE_CONFIRMATION_URL_REQUEST_BUILDER_H_  // NOLINT

#include <string>
#include <vector>

#include "bat/ads/internal/confirmations/confirmation_info.h"
#include "bat/ads/internal/server/url_request_builder.h"

namespace ads {

namespace privacy {
struct UnblindedTokenInfo;
}  // privacy

class CreateConfirmationUrlRequestBuilder : UrlRequestBuilder {
 public:
  CreateConfirmationUrlRequestBuilder(
      const ConfirmationInfo& confirmation);

  ~CreateConfirmationUrlRequestBuilder() override;

  UrlRequestPtr Build() override;

 private:
  ConfirmationInfo confirmation_;

  std::string BuildUrl() const;

  std::vector<std::string> BuildHeaders() const;

  std::string BuildBody() const;
};

}  // namespace ads

#endif  // BAT_ADS_INTERNAL_TOKENS_REDEEM_UNBLINDED_TOKEN_CREATE_CONFIRMATION_URL_REQUEST_BUILDER_H_  // NOLINT
