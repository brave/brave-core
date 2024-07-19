/* Copyright (c) 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "third_party/blink/public/common/client_hints/enabled_client_hints.h"
#include "third_party/blink/public/common/features.h"

#define SetIsEnabled SetIsEnabled_ChromiumImpl

#include "src/third_party/blink/common/client_hints/enabled_client_hints.cc"

#undef SetIsEnabled

// By default we will send three (3) non-privacy-risking CHs: kUA, kUAMobile,
// and kUAPlatform.
// Additionally:
//   - if we receive CH requests for kUAArch, kUABitness, kUAFullVersionList, or
//   kUAWoW64, we will send these.
//   - if we receive CH requests for kUAPlatformVersion and/or kUAModel, we will
//   send these, too, but:
//     - kUAModel will be always set to an empty string;
//     - kUAPlatformVersion will be clamped to the same value we report in the
//     User-Agent string.

namespace blink {

void EnabledClientHints::SetIsEnabled(const WebClientHintsType type,
                                      const bool should_send) {
  bool type_is_enabled = false;
  switch (type) {
    case WebClientHintsType::kUA:
    case WebClientHintsType::kUAArch:
    case WebClientHintsType::kUABitness:
    case WebClientHintsType::kUAFullVersionList:
    case WebClientHintsType::kUAMobile:
    case WebClientHintsType::kUAModel:
    case WebClientHintsType::kUAPlatform:
    case WebClientHintsType::kUAPlatformVersion:
    case WebClientHintsType::kUAWoW64:
      type_is_enabled = true;
      break;
    default:
      break;
  }

  if (type_is_enabled) {
    SetIsEnabled_ChromiumImpl(type, should_send);
  } else {
    enabled_types_[static_cast<int>(type)] = false;
  }
}

}  // namespace blink
