/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include "../include/platform_helper.h"

#include "../deps/bat-native-rapidjson/include/rapidjson/document.h"
#include "../deps/bat-native-rapidjson/include/rapidjson/error/en.h"
#include "../deps/bat-native-rapidjson/include/rapidjson/stringbuffer.h"
#include "../deps/bat-native-rapidjson/include/rapidjson/writer.h"

static const char* _rapidjson_member_types[] = {
  "Null",
  "Bool",  // False
  "Bool",  // True
  "Object",
  "Array",
  "String",
  "Number"};

namespace state {

struct USER_MODEL_STATE;
struct CATALOG_STATE;

using JsonWriter = rapidjson::Writer<rapidjson::StringBuffer>;

void SaveToJson(JsonWriter& writer, const USER_MODEL_STATE& state);
void SaveToJson(JsonWriter& writer, const CATALOG_STATE& state);

template <typename T>
void SaveToJsonString(const T& t, std::string& json) {
  rapidjson::StringBuffer buffer;
  JsonWriter writer(buffer);

  SaveToJson(writer, t);
  json = buffer.GetString();
}

// return: parsing status: true = succeeded, false = failed
template <typename T>
bool LoadFromJson(T& t, const std::string& json) {
  bool succeeded = t.LoadFromJson(json);
  if (!succeeded) {
    LOG(ERROR) << "Failed to load JSON: " << json << std::endl;
  }

  return succeeded;
}

}  // namespace state
