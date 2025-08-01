/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/p3a/features.h"

namespace p3a::features {

// Verify Constellation randomness server secure enclave certificate.
BASE_FEATURE(kConstellationEnclaveAttestation,
             "BraveP3AConstellationEnclaveAttestation",
             base::FEATURE_DISABLED_BY_DEFAULT);

bool IsConstellationEnclaveAttestationEnabled() {
  return base::FeatureList::IsEnabled(
      features::kConstellationEnclaveAttestation);
}

}  // namespace p3a::features
