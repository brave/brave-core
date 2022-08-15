/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/user_data/conversion_user_data_builder.h"

#include <utility>

#include "base/check.h"
#include "bat/ads/internal/account/user_data/conversion_user_data_util.h"
#include "bat/ads/internal/conversions/conversion_queue_database_table.h"
#include "bat/ads/internal/conversions/conversion_queue_item_info.h"
#include "bat/ads/internal/conversions/verifiable_conversion_envelope_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {
namespace user_data {
namespace builder {

namespace {

constexpr char kAlgorithmKey[] = "alg";
constexpr char kAlgorithm[] = "crypto_box_curve25519xsalsa20poly1305";
constexpr char kCipherTextKey[] = "ciphertext";
constexpr char kEphemeralPublicKeyKey[] = "epk";
constexpr char kNonceKey[] = "nonce";
constexpr char kConversionEnvelopeKey[] = "conversionEnvelope";

void ReportConversionDoesNotExist(BuildConversionCallback callback) {
  callback(base::Value::Dict());
}

}  // namespace

void BuildConversion(const std::string& creative_instance_id,
                     BuildConversionCallback callback) {
  DCHECK(!creative_instance_id.empty());

  database::table::ConversionQueue database_table;
  database_table.GetForCreativeInstanceId(
      creative_instance_id,
      [callback](const bool success, const std::string& creative_instance_id,
                 const ConversionQueueItemList& conversion_queue_items) {
        if (!success) {
          ReportConversionDoesNotExist(callback);
          return;
        }

        if (conversion_queue_items.empty()) {
          ReportConversionDoesNotExist(callback);
          return;
        }

        const ConversionQueueItemInfo& conversion_queue_item =
            conversion_queue_items.front();
        const absl::optional<security::VerifiableConversionEnvelopeInfo>
            verifiable_conversion_envelope = GetEnvelope(conversion_queue_item);
        if (!verifiable_conversion_envelope) {
          ReportConversionDoesNotExist(callback);
          return;
        }

        base::Value::Dict conversion_envelope;
        conversion_envelope.Set(kAlgorithmKey, kAlgorithm);
        conversion_envelope.Set(kCipherTextKey,
                                verifiable_conversion_envelope->ciphertext);
        conversion_envelope.Set(
            kEphemeralPublicKeyKey,
            verifiable_conversion_envelope->ephemeral_public_key);
        conversion_envelope.Set(kNonceKey,
                                verifiable_conversion_envelope->nonce);

        base::Value::Dict user_data;
        user_data.Set(kConversionEnvelopeKey, std::move(conversion_envelope));

        callback(std::move(user_data));
      });
}

}  // namespace builder
}  // namespace user_data
}  // namespace ads
