/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/webcompat_reporter/browser/fields.h"

#include "base/notreached.h"
#include "brave/components/brave_shields/common/brave_shields_panel.mojom-shared.h"

namespace webcompat_reporter {

namespace {

const char kAggressive[] = "aggressive";
const char kStandard[] = "standard";
const char kAllow[] = "allow";

}  // namespace

const char kFPBlockSettingField[] = "fpBlockSetting";
const char kAdBlockSettingField[] = "adBlockSetting";
const char kAdBlockListsField[] = "adBlockLists";
const char kShieldsEnabledField[] = "shieldsEnabled";
const char kLanguagesField[] = "languages";
const char kLanguageFarblingField[] = "languageFarblingEnabled";
const char kBraveVPNEnabledField[] = "braveVPNEnabled";

const char kSiteURLField[] = "url";
const char kDomainField[] = "domain";
const char kDetailsField[] = "additionalDetails";
const char kContactField[] = "contactInfo";
const char kApiKeyField[] = "api_key";

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
  NOTREACHED();
}

const char* GetFingerprintModeString(
    brave_shields::mojom::FingerprintMode fp_block_mode) {
  switch (fp_block_mode) {
    case brave_shields::mojom::FingerprintMode::STRICT:
      return kAggressive;
    case brave_shields::mojom::FingerprintMode::STANDARD:
      return kStandard;
    case brave_shields::mojom::FingerprintMode::ALLOW:
      return kAllow;
  }
  NOTREACHED();
}

}  // namespace webcompat_reporter
