/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_P3A_FEATURES_H_
#define BRAVE_COMPONENTS_P3A_FEATURES_H_

#include "base/feature_list.h"

namespace p3a {
namespace features {

// See https://github.com/brave/brave-browser/issues/24338 for more info.
BASE_DECLARE_FEATURE(kConstellation);
// See https://github.com/brave/brave-browser/issues/31718 for more info.
BASE_DECLARE_FEATURE(kConstellationEnclaveAttestation);

bool IsConstellationEnabled();
bool IsConstellationEnclaveAttestationEnabled();

}  // namespace features
}  // namespace p3a

#endif  // BRAVE_COMPONENTS_P3A_FEATURES_H_
