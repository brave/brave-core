/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "brave/components/content_settings/core/browser/brave_content_settings_utils.h"

#define BRAVE_SHIELDS_DICTIONARY_NAME                 \
const char kBraveShieldsDictionary[] = "brave_shields";

#define BRAVE_SET_WEBSITE_SETTING                                        \
         content_settings::IsShieldsContentSettingsType(content_type_) ||

#define BRAVE_READ_CONTENT_SETTINGS_FROM_PREF                                  \
    if (content_type_ == ContentSettingsType::PLUGINS ||                       \
        content_settings::IsShieldsContentSettingsType(content_type_)) {       \
      const base::DictionaryValue* resource_dictionary = nullptr;              \
      if (settings_dictionary->GetDictionary(kBraveShieldsDictionary,          \
                                             &resource_dictionary)) {          \
        base::Time last_modified = GetTimeStamp(settings_dictionary);          \
        for (base::DictionaryValue::Iterator j(*resource_dictionary);          \
             !j.IsAtEnd(); j.Advance()) {                                      \
          int setting = CONTENT_SETTING_DEFAULT;                               \
          bool is_integer = j.value().GetAsInteger(&setting);                  \
          DCHECK(is_integer);                                                  \
          DCHECK_NE(CONTENT_SETTING_DEFAULT, setting);                         \
          std::unique_ptr<base::Value> setting_ptr(new base::Value(setting));  \
          DCHECK(IsValueAllowedForType(setting_ptr.get(), content_type_));     \
          value_map_.SetValue(pattern_pair.first, pattern_pair.second,         \
                              content_type_, last_modified,                    \
                              setting_ptr->Clone(),                            \
                              {expiration, session_model});                    \
        }                                                                      \
      }                                                                        \
    }

#define BRAVE_UPDATE_PREF_HANDLE_SHIELDS_TYPE                                 \
    if (content_settings::IsShieldsContentSettingsType(content_type_)) {      \
      std::unique_ptr<prefs::DictionaryValueUpdate> resource_dictionary;      \
      found = settings_dictionary->GetDictionary(                             \
          kBraveShieldsDictionary, &resource_dictionary);                     \
      if (!found) {                                                           \
        if (value == nullptr)                                                 \
          return;                                                             \
        resource_dictionary =                                                 \
            settings_dictionary->SetDictionaryWithoutPathExpansion(           \
                kBraveShieldsDictionary,                                      \
                std::make_unique<base::DictionaryValue>());                   \
      }                                                                       \
      if (value == nullptr) {                                                 \
        resource_dictionary->RemoveWithoutPathExpansion(                      \
            GetShieldsContentTypeName(content_type_), nullptr);               \
        if (resource_dictionary->empty()) {                                   \
          settings_dictionary->RemoveWithoutPathExpansion(                    \
              kBraveShieldsDictionary, nullptr);                              \
          settings_dictionary->RemoveWithoutPathExpansion(kLastModifiedPath,  \
                                                          nullptr);           \
          settings_dictionary->RemoveWithoutPathExpansion(kExpirationPath,    \
                                                          nullptr);           \
          settings_dictionary->RemoveWithoutPathExpansion(kSessionModelPath,  \
                                                          nullptr);           \
        }                                                                     \
      } else {                                                                \
        resource_dictionary->SetWithoutPathExpansion(                         \
            GetShieldsContentTypeName(content_type_),                         \
            value->CreateDeepCopy());                                         \
        settings_dictionary->SetKey(                                          \
            kLastModifiedPath,                                                \
            base::Value(base::NumberToString(                                 \
                last_modified.ToDeltaSinceWindowsEpoch().InMicroseconds()))); \
        settings_dictionary->SetKey(                                          \
            kExpirationPath,                                                  \
            base::Value(base::NumberToString(                                 \
                constraints.expiration.ToDeltaSinceWindowsEpoch()             \
                    .InMicroseconds())));                                     \
        settings_dictionary->SetKey(                                          \
            kSessionModelPath,                                                \
            base::Value(static_cast<int>(constraints.session_model)));        \
      }                                                                       \
    } else {

#define BRAVE_UPDATE_PREF_HANDLE_SHIELDS_TYPE_END \
    }

#include "../../../../../../components/content_settings/core/browser/content_settings_pref.cc"

#undef BRAVE_SHIELDS_DICTIONARY_NAME
#undef BRAVE_SET_WEBSITE_SETTING
#undef BRAVE_READ_CONTENT_SETTINGS_FROM_PREF
#undef BRAVE_UPDATE_PREF_HANDLE_SHIELDS_TYPE
#undef BRAVE_UPDATE_PREF_HANDLE_SHIELDS_TYPE_END
