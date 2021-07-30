/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/crypto_dot_com/browser/crypto_dot_com_pref_utils.h"

#include "components/prefs/pref_registry_simple.h"
#include "brave/components/crypto_dot_com/common/pref_names.h"

namespace crypto_dot_com {

void RegisterProfilePrefs(PrefRegistrySimple* registry) {
  registry->RegisterBooleanPref(kCryptoDotComNewTabPageShowCryptoDotCom, true);
  registry->RegisterBooleanPref(kCryptoDotComHasBoughtCrypto, false);
  registry->RegisterBooleanPref(kCryptoDotComHasInteracted, false);
}

}  // namespace crypto_dot_com
