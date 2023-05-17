/* Copyright (c) 2022 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "brave/components/brave_wallet/common/common_util.h"

#include "brave/components/brave_wallet/common/pref_names.h"
#include "build/build_config.h"
#include "components/prefs/pref_service.h"

namespace brave_wallet {

namespace {

bool IsDisabledByPolicy(PrefService* prefs) {
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
  DCHECK(prefs);
  return prefs->IsManagedPreference(prefs::kDisabledByPolicy) &&
         prefs->GetBoolean(prefs::kDisabledByPolicy);
#else
  return false;
#endif  // BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC) || BUILDFLAG(IS_LINUX)
}

}  // namespace

bool IsAllowed(PrefService* prefs) {
  return !IsDisabledByPolicy(prefs);
}

std::vector<uint8_t> Leb128Encode(size_t value) {
  value |= 0;
  std::vector<uint8_t> result;
  while (true) {
    uint8_t byte = value & 0x7f;
    value >>= 7;
    if ((value == 0 && (byte & 0x40) == 0)) {
      result.push_back(byte);
      return result;
    }
    result.push_back(byte | 0x80);
  }
}

}  // namespace brave_wallet
