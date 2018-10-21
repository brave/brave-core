/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include "bat-native-rapidjson/include/rapidjson/document.h"
#include "bat-native-rapidjson/include/rapidjson/error/en.h"
#include "bat-native-rapidjson/include/rapidjson/stringbuffer.h"
#include "bat-native-rapidjson/include/rapidjson/writer.h"

// TODO(Terry Mancey): Decouple _rapidjson_member_types
static const char* _rapidjson_member_types[] = {
  "Null",
  "Bool",  // False
  "Bool",  // True
  "Object",
  "Array",
  "String",
  "Number"};

namespace state {

struct CLIENT_STATE;
struct BUNDLE_STATE;

using JsonWriter = rapidjson::Writer<rapidjson::StringBuffer>;

void SaveToJson(JsonWriter& writer, const CLIENT_STATE& state);
void SaveToJson(JsonWriter& writer, const BUNDLE_STATE& state);

template <typename T>
void SaveToJson(const T& t, std::string& json) {
  rapidjson::StringBuffer buffer;
  JsonWriter writer(buffer);

  SaveToJson(writer, t);
  json = buffer.GetString();
}

// return: parsing status: true = succeeded, false = failed
template <typename T>
bool LoadFromJson(T& t, const std::string& json) {
  return t.LoadFromJson(json);
}

}  // namespace state
