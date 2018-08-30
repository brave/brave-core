/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */
#ifndef BRAVE_COMPONENTS_BRAVE_SYNC_VALUES_CONV_H_
#define BRAVE_COMPONENTS_BRAVE_SYNC_VALUES_CONV_H_

#include <memory>
#include <vector>
#include <string>

#include <base/time/time.h>

namespace base {
  class Value;
}

namespace bookmarks {
  class BookmarkNode;
}

namespace brave_sync {

class Settings;

std::unique_ptr<base::Value> BraveSyncSettingsToValue(brave_sync::Settings *brave_sync_settings);

std::string ExtractObjectIdFromDict(const base::Value *val);
std::string ExtractObjectIdFromList(const base::Value *val);

std::string ExtractDeviceIdFromDict(const base::Value *val);

std::string ExtractIdFieldFromDictOrList(const base::Value *val, const std::string &fileld_name);

base::Time ExtractTimeFieldFromDict(const base::Value *val, const std::string &time_fileld_name);

bool ExtractBool(const base::Value *val, const std::string &fileld_name);

template <typename TEnum>
TEnum ExtractEnum(const base::Value *val, const std::string &fileld_name,
  TEnum min, TEnum max, TEnum def);

template <typename TEnum>
TEnum ConvertEnum(const int val, TEnum min, TEnum max, TEnum def);


using Uint8Array = std::vector<unsigned char>;
Uint8Array Uint8ArrayFromSignedCharVec(const std::vector<char> &vec);
std::string StrFromUint8Array(const Uint8Array &arr);
std::string StrFromCharArray(const std::vector<char> &vec);
Uint8Array Uint8ArrayFromIntVec(const std::vector<int> vec);
Uint8Array Uint8ArrayFromString(const std::string &data_string);
std::vector<int> IntVecFromString(const std::string &data_string);
std::vector<char> CharVecFromString(const std::string &data_string);
std::unique_ptr<base::Value> VecToListValue(const std::vector<char> &v);
std::unique_ptr<base::Value> BytesListFromString(const std::string &data_string);
std::unique_ptr<base::Value> SingleIntStrToListValue(const std::string &data_string);

std::unique_ptr<base::Value> BlobFromString(const std::string &data_string);
std::unique_ptr<base::Value> BlobFromSingleIntStr(const std::string &data_string);

std::string GetAction(const base::Value &sync_record);
int GetIntAction(const base::Value &sync_record);

} // namespace brave_sync

#endif //BRAVE_COMPONENTS_BRAVE_SYNC_VALUES_CONV_H_
