/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/content/browser/order_fetcher.h"

#include <optional>
#include <string>
#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/numerics/safe_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "brave/components/brave_ads/content/browser/order_fetch_helper.h"
#include "brave/components/brave_ads/content/browser/order_fetcher_feature.h"
#include "brave/components/brave_ads/content/browser/order_patterns_mojom_util.h"
#include "brave/components/brave_ads/content/common/mojom/order_extractor.mojom.h"
#include "brave/components/brave_ads/core/mojom/brave_ads.mojom.h"
#include "components/autofill/core/common/credit_card_number_validation.h"
#include "content/public/browser/web_contents.h"
#include "third_party/re2/src/re2/re2.h"

namespace brave_ads {

namespace {

// Resets the extracted order field if it exceeds the maximum allowed length.
void ResetExtractedOrderFieldIfExceedsMaxLength(
    std::optional<std::string>& extracted_order_field) {
  if (extracted_order_field &&
      extracted_order_field->size() >
          base::saturated_cast<size_t>(kMaxExtractedOrderFieldLength.Get())) {
    extracted_order_field.reset();
  }
}

// Validates the extracted order information against the provided patterns and
// applies additional security checks before invoking the callback.
void ValidateExtractedOrderCallback(
    std::optional<std::string> id_validation_pattern,
    FetchOrderCallback callback,
    bool should_extract_id,
    bool should_extract_value,
    mojom::ExtractedOrderInfoPtr mojom_extracted_order) {
  CHECK(mojom_extracted_order);

  ResetExtractedOrderFieldIfExceedsMaxLength(mojom_extracted_order->id);
  ResetExtractedOrderFieldIfExceedsMaxLength(mojom_extracted_order->value);
  ResetExtractedOrderFieldIfExceedsMaxLength(mojom_extracted_order->currency);

  if (mojom_extracted_order->id && id_validation_pattern) {
    const re2::RE2 regex(*id_validation_pattern, re2::RE2::Quiet);
    if (!regex.ok() ||
        !re2::RE2::FullMatch(*mojom_extracted_order->id, regex)) {
      // ID does not match the validation pattern, so discard it.
      mojom_extracted_order->id.reset();
    }
  }

  if (mojom_extracted_order->id &&
      autofill::IsValidCreditCardNumber(
          base::UTF8ToUTF16(*mojom_extracted_order->id))) {
    // ID looks like a credit card number, which is sensitive, so discard it.
    mojom_extracted_order->id.reset();
  }

  if (mojom_extracted_order->value) {
    const re2::RE2 value_regex(kOrderValueValidationPattern.Get(),
                               re2::RE2::Quiet);
    if (!value_regex.ok() ||
        !re2::RE2::FullMatch(*mojom_extracted_order->value, value_regex) ||
        autofill::IsValidCreditCardNumber(
            base::UTF8ToUTF16(*mojom_extracted_order->value))) {
      // Value does not match the validation pattern or looks like a credit card
      // number, so discard it.
      mojom_extracted_order->value.reset();
      mojom_extracted_order->currency.reset();
    }
  } else {
    // If the value is missing, the currency is meaningless, so discard it.
    mojom_extracted_order->currency.reset();
  }

  std::move(callback).Run(should_extract_id, should_extract_value,
                          std::move(mojom_extracted_order));
}

}  // namespace

OrderFetcher::OrderFetcher(content::WebContents& web_contents)
    : web_contents_(web_contents) {}

OrderFetcher::~OrderFetcher() = default;

void OrderFetcher::FetchOrder(
    const mojom::OrderExtractionPatternsInfo& mojom_patterns,
    FetchOrderCallback callback) {
  // Replacing the previous helper, if any, destroys its mojo remote and
  // timer, abandoning the previous fetch before it can invoke `callback`.
  order_fetch_helper_ = std::make_unique<OrderFetchHelper>();

  order_fetch_helper_->FetchOrder(
      *web_contents_->GetPrimaryMainFrame(),
      ToMojomOrderPatterns(mojom_patterns), kFetchOrderTimeout.Get(),
      base::BindOnce(&ValidateExtractedOrderCallback,
                     mojom_patterns.id ? mojom_patterns.id->validation_pattern
                                       : std::nullopt,
                     std::move(callback), mojom_patterns.should_extract_id,
                     mojom_patterns.should_extract_value));
}

}  // namespace brave_ads
