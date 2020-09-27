/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/crypto_dot_com/browser/crypto_dot_com_pref_utils.h"

#include "brave/components/crypto_dot_com/common/pref_names.h"
#include "components/prefs/pref_registry_simple.h"

namespace crypto_dot_com {

CryptoDotComPrefUtils::CryptoDotComPrefUtils() {}

CryptoDotComPrefUtils::~CryptoDotComPrefUtils() {}

// static
void CryptoDotComPrefUtils::RegisterPrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(kCryptoDotComNewTabPageShowCryptoDotCom, true);
}

}  // namespace crypto_dot_com
