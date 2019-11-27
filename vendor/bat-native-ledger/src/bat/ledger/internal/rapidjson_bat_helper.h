/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_RAPIDJSON_BAT_HELPER_H_
#define BRAVELEDGER_RAPIDJSON_BAT_HELPER_H_

#include <string>

#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

namespace braveledger_bat_helper {

using JsonWriter = rapidjson::Writer<rapidjson::StringBuffer>;

template <typename T>
void saveToJsonString(const T& t, std::string* json) {
  rapidjson::StringBuffer buffer;
  JsonWriter writer(buffer);
  saveToJson(&writer, t);
  *json = buffer.GetString();
}

// return: parsing status: true = succeed, false = failed
template <typename T>
bool loadFromJson(T* t, const std::string& json) {
  return t->loadFromJson(json);
}

}  // namespace braveledger_bat_helper

#endif  // BRAVELEDGER_RAPIDJSON_BAT_HELPER_H_
