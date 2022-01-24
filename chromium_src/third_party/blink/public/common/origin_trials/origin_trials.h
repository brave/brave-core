/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_PUBLIC_COMMON_ORIGIN_TRIALS_ORIGIN_TRIALS_H_
#define BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_PUBLIC_COMMON_ORIGIN_TRIALS_ORIGIN_TRIALS_H_

#include "src/third_party/blink/public/common/origin_trials/origin_trials.h"

namespace blink {
namespace origin_trials {

BLINK_COMMON_EXPORT bool IsTrialDisabledInBrave(base::StringPiece trial_name);
BLINK_COMMON_EXPORT bool IsTrialDisabledInBrave(OriginTrialFeature feature);

}  // namespace origin_trials
}  // namespace blink

#endif  // BRAVE_CHROMIUM_SRC_THIRD_PARTY_BLINK_PUBLIC_COMMON_ORIGIN_TRIALS_ORIGIN_TRIALS_H_
