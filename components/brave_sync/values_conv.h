/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_VALUES_CONV_H_
#define BRAVE_COMPONENTS_BRAVE_SYNC_VALUES_CONV_H_

#include <memory>
#include <vector>
#include <string>

#include <base/time/time.h>
#include "brave/components/brave_sync/jslib_messages_fwd.h"

namespace base {
  class Value;
}

namespace brave_sync {

class Settings;

std::unique_ptr<base::Value> BraveSyncSettingsToValue(brave_sync::Settings *brave_sync_settings);

template <typename TEnum>
TEnum ExtractEnum(const base::Value *val, const std::string &fileld_name,
  TEnum min, TEnum max, TEnum def);

template <typename TEnum>
TEnum ConvertEnum(const int val, TEnum min, TEnum max, TEnum def);

std::string StrFromUint8Array(const Uint8Array &arr);
std::string StrFromUnsignedCharArray(const std::vector<unsigned char> &vec);
Uint8Array Uint8ArrayFromString(const std::string &data_string);
std::vector<unsigned char> UCharVecFromString(const std::string &data_string);

} // namespace brave_sync

#endif //BRAVE_COMPONENTS_BRAVE_SYNC_VALUES_CONV_H_
