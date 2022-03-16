/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/account/user_data/conversion_user_data_builder.h"

#include "base/check.h"
#include "base/values.h"
#include "bat/ads/internal/account/user_data/conversion_user_data_util.h"
#include "bat/ads/internal/conversions/conversion_queue_item_info.h"
#include "bat/ads/internal/database/tables/conversion_queue_database_table.h"
#include "bat/ads/internal/logging.h"
#include "bat/ads/internal/security/conversions/verifiable_conversion_envelope_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace ads {
namespace user_data {

namespace {

constexpr char kAlgorithmKey[] = "alg";
constexpr char kAlgorithm[] = "crypto_box_curve25519xsalsa20poly1305";
constexpr char kCipherTextKey[] = "ciphertext";
constexpr char kEphemeralPublicKeyKey[] = "epk";
constexpr char kNonceKey[] = "nonce";
constexpr char kConversionEnvelopeKey[] = "conversionEnvelope";

void ReportConversionDoesNotExist(ConversionCallback callback) {
  callback(base::DictionaryValue());
}

}  // namespace

void BuildConversion(const std::string& creative_instance_id,
                     ConversionCallback callback) {
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
        const absl::optional<security::VerifiableConversionEnvelopeInfo>&
            verifiable_conversion_envelope_optional =
                GetEnvelope(conversion_queue_item);
        if (!verifiable_conversion_envelope_optional) {
          ReportConversionDoesNotExist(callback);
          return;
        }
        const security::VerifiableConversionEnvelopeInfo&
            verifiable_conversion_envelope =
                verifiable_conversion_envelope_optional.value();

        base::DictionaryValue conversion_envelope;
        conversion_envelope.SetKey(kAlgorithmKey, base::Value(kAlgorithm));
        conversion_envelope.SetKey(
            kCipherTextKey,
            base::Value(verifiable_conversion_envelope.ciphertext));
        conversion_envelope.SetKey(
            kEphemeralPublicKeyKey,
            base::Value(verifiable_conversion_envelope.ephemeral_public_key));
        conversion_envelope.SetKey(
            kNonceKey, base::Value(verifiable_conversion_envelope.nonce));

        base::DictionaryValue user_data;
        user_data.SetKey(kConversionEnvelopeKey, conversion_envelope.Clone());

        callback(user_data.Clone());
      });
}

}  // namespace user_data
}  // namespace ads
