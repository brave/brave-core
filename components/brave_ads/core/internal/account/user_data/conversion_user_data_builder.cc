/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_ads/core/internal/account/user_data/conversion_user_data_builder.h"

#include <utility>

#include "base/check.h"
#include "base/functional/bind.h"
#include "base/functional/callback.h"
#include "brave/components/brave_ads/core/internal/account/user_data/conversion_user_data_util.h"
#include "brave/components/brave_ads/core/internal/conversions/conversion_queue_database_table.h"
#include "brave/components/brave_ads/core/internal/conversions/conversion_queue_item_info.h"
#include "brave/components/brave_ads/core/internal/conversions/conversions_util.h"
#include "brave/components/brave_ads/core/internal/conversions/verifiable_conversion_envelope_constants.h"
#include "brave/components/brave_ads/core/internal/conversions/verifiable_conversion_envelope_info.h"
#include "third_party/abseil-cpp/absl/types/optional.h"

namespace brave_ads {

void BuildVerifiableConversionUserData(
    const std::string& creative_instance_id,
    BuildVerifiableConversionUserDataCallback callback) {
  CHECK(!creative_instance_id.empty());

  const database::table::ConversionQueue database_table;
  database_table.GetForCreativeInstanceId(
      creative_instance_id,
      base::BindOnce(
          [](BuildVerifiableConversionUserDataCallback callback,
             const bool success, const std::string& /*creative_instance_id*/,
             const ConversionQueueItemList& conversion_queue_items) {
            if (!success) {
              return std::move(callback).Run(/*user_data*/ {});
            }

            if (conversion_queue_items.empty()) {
              return std::move(callback).Run(/*user_data*/ {});
            }

            const ConversionQueueItemInfo& conversion_queue_item =
                conversion_queue_items.front();
            const absl::optional<VerifiableConversionEnvelopeInfo>
                verifiable_conversion_envelope =
                    MaybeBuildVerifiableConversionEnvelope(
                        conversion_queue_item);
            if (!verifiable_conversion_envelope) {
              return std::move(callback).Run(/*user_data*/ {});
            }

            base::Value::Dict dict;
            dict.Set(kVerifiableConversionEnvelopeAlgorithmKey, GetAlgorithm());
            dict.Set(kVerifiableConversionEnvelopeCipherTextKey,
                     verifiable_conversion_envelope->ciphertext);
            dict.Set(kVerifiableConversionEnvelopeEphemeralPublicKeyKey,
                     verifiable_conversion_envelope->ephemeral_public_key);
            dict.Set(kVerifiableConversionEnvelopeNonceKey,
                     verifiable_conversion_envelope->nonce);

            base::Value::Dict user_data;
            user_data.Set(kVerifiableConversionEnvelopeKey, std::move(dict));

            std::move(callback).Run(std::move(user_data));
          },
          std::move(callback)));
}

}  // namespace brave_ads
