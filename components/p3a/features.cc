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
// Report P3A responses with "Nebula" differential privacy
// sampling enabled. See https://github.com/brave/brave-browser/issues/35841
BASE_FEATURE(kNebula,
             "BraveP3ADifferentialSampling",
             base::FEATURE_DISABLED_BY_DEFAULT);

bool IsConstellationEnclaveAttestationEnabled() {
  return base::FeatureList::IsEnabled(
      features::kConstellationEnclaveAttestation);
}

bool IsNebulaEnabled() {
  return base::FeatureList::IsEnabled(features::kNebula);
}

}  // namespace p3a::features
