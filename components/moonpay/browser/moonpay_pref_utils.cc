/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/moonpay/browser/moonpay_pref_utils.h"

#include "brave/components/moonpay/common/moonpay_pref_names.h"
#include "components/prefs/pref_registry_simple.h"

namespace moonpay {

MoonpayPrefUtils::MoonpayPrefUtils() {}

MoonpayPrefUtils::~MoonpayPrefUtils() {}

// static
void MoonpayPrefUtils::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(kMoonpayNewTabPageShowBitcoinDotCom, true);
  registry->RegisterBooleanPref(kMoonpayHasBoughtBitcoinDotComCrypto, false);
  registry->RegisterBooleanPref(kMoonpayHasInteractedBitcoinDotCom, false);
}

}  // namespace moonpay
