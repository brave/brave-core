/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/webcompat_reporter/browser/fields.h"

#include "base/notreached.h"
#include "brave/components/brave_shields/core/common/brave_shields_panel.mojom-shared.h"

namespace webcompat_reporter {

namespace {

const char kAggressive[] = "aggressive";
const char kStandard[] = "standard";
const char kAllow[] = "allow";

}  // namespace

const char* GetAdBlockModeString(
    brave_shields::mojom::AdBlockMode ad_block_mode) {
  switch (ad_block_mode) {
    case brave_shields::mojom::AdBlockMode::AGGRESSIVE:
      return kAggressive;
    case brave_shields::mojom::AdBlockMode::STANDARD:
      return kStandard;
    case brave_shields::mojom::AdBlockMode::ALLOW:
      return kAllow;
  }
  NOTREACHED_IN_MIGRATION();
}

const char* GetFingerprintModeString(
    brave_shields::mojom::FingerprintMode fp_block_mode) {
  switch (fp_block_mode) {
    case brave_shields::mojom::FingerprintMode::STRICT_MODE:
      return kAggressive;
    case brave_shields::mojom::FingerprintMode::STANDARD_MODE:
      return kStandard;
    case brave_shields::mojom::FingerprintMode::ALLOW_MODE:
      return kAllow;
  }
  NOTREACHED_IN_MIGRATION();
}

}  // namespace webcompat_reporter
