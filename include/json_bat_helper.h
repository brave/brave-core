/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#pragma once

#include "../deps/bat-native-rapidjson/include/rapidjson/document.h"
#include "../deps/bat-native-rapidjson/include/rapidjson/error/en.h"
#include "../deps/bat-native-rapidjson/include/rapidjson/stringbuffer.h"
#include "../deps/bat-native-rapidjson/include/rapidjson/writer.h"

namespace ads_bat_client {

struct USER_MODEL_STATE_ST;

using JsonWriter = rapidjson::Writer<rapidjson::StringBuffer>;

void SaveToJson(JsonWriter& writer, const USER_MODEL_STATE_ST& state);

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
    // TODO(Terry Mancey): Implement LOG() (#39)
    // LOG(ERROR) << "Failed to parse: " << json << std::endl;
  }

  return succeeded;
}

}  // namespace ads_bat_client
