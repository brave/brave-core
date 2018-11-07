/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"
#include "rapidjson/schema.h"

namespace ads {

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

template <typename T>
bool LoadFromJson(T& t, const std::string& json) {
  return t.LoadFromJson(json);
}

template <typename T>
bool LoadFromJson(
    T& t,
    const std::string& json,
    const std::string& jsonSchema) {
  return t.LoadFromJson(json, jsonSchema);
}

}  // namespace ads

namespace helper {

class JSON {
 public:
  static bool Validate(
      rapidjson::Document *document,
      const std::string& jsonSchema);
};

}  // namespace helper
