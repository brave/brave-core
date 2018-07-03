/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_RAPIDJSON_BAT_HELPER_H_
#define BRAVELEDGER_RAPIDJSON_BAT_HELPER_H_

#include "rapidjson/document.h"
#include "rapidjson/error/en.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

namespace braveledger_bat_helper {

struct BALLOT_ST;
struct CLIENT_STATE_ST;
struct MEDIA_PUBLISHER_INFO;
struct PUBLISHER_ST;
struct PUBLISHER_STATE_ST;
struct SURVEYOR_ST;
struct TRANSACTION_BALLOT_ST;
struct TRANSACTION_ST;
struct TWITCH_EVENT_INFO;
struct WALLET_INFO_ST;

using JsonWriter = rapidjson::Writer<rapidjson::StringBuffer>;

void saveToJson(JsonWriter & writer, const BALLOT_ST&);
void saveToJson(JsonWriter & writer, const CLIENT_STATE_ST&);
void saveToJson(JsonWriter & writer, const MEDIA_PUBLISHER_INFO&);
void saveToJson(JsonWriter & writer, const PUBLISHER_ST&);
void saveToJson(JsonWriter & writer, const PUBLISHER_STATE_ST&);
void saveToJson(JsonWriter & writer, const SURVEYOR_ST&);
void saveToJson(JsonWriter & writer, const TRANSACTION_BALLOT_ST&);
void saveToJson(JsonWriter & writer, const TRANSACTION_ST&);
void saveToJson(JsonWriter & writer, const TWITCH_EVENT_INFO&);
void saveToJson(JsonWriter & writer, const WALLET_INFO_ST&);

template <typename T>
void saveToJsonString(const T& t, std::string& json) {
  rapidjson::StringBuffer buffer;
  JsonWriter writer(buffer);
  saveToJson(writer, t);
  json = buffer.GetString();
}

//return: parsing status:  true = succeded, false = failed
template <typename T>
bool loadFromJson(T& t, const std::string& json) {
  bool succeded = t.loadFromJson(json);
  if (!succeded) {
    LOG(ERROR) << "Failed to parse:" << json << std::endl;
  }
  return succeded;
}

}  // namespace braveledger_bat_helper

#endif  // BRAVELEDGER_RAPIDJSON_BAT_HELPER_H_
