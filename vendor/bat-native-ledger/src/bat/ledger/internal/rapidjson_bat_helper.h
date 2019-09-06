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

struct BALLOT_ST;
struct MEDIA_PUBLISHER_INFO;
struct PUBLISHER_ST;
struct PUBLISHER_STATE_ST;
struct RECONCILE_DIRECTION;
struct CURRENT_RECONCILE;
struct CLIENT_STATE_ST;
struct TRANSACTION_BALLOT_ST;
struct TRANSACTION_ST;
struct TWITCH_EVENT_INFO;
struct WALLET_INFO_ST;
struct WALLET_PROPERTIES_ST;
struct GRANT;

using JsonWriter = rapidjson::Writer<rapidjson::StringBuffer>;

void saveToJson(JsonWriter* writer, const BALLOT_ST&);
void saveToJson(JsonWriter* writer, const MEDIA_PUBLISHER_INFO&);
void saveToJson(JsonWriter* writer, const PUBLISHER_ST&);
void saveToJson(JsonWriter* writer, const PUBLISHER_STATE_ST&);
void saveToJson(JsonWriter* writer, const RECONCILE_DIRECTION&);
void saveToJson(JsonWriter* writer, const CURRENT_RECONCILE&);
void saveToJson(JsonWriter* writer, const CLIENT_STATE_ST&);
void saveToJson(JsonWriter* writer, const TRANSACTION_BALLOT_ST&);
void saveToJson(JsonWriter* writer, const TRANSACTION_ST&);
void saveToJson(JsonWriter* writer, const TWITCH_EVENT_INFO&);
void saveToJson(JsonWriter* writer, const WALLET_INFO_ST&);
void saveToJson(JsonWriter* writer, const GRANTS_PROPERTIES_ST&);
void saveToJson(JsonWriter* writer, const ledger::Grant&);
void saveToJson(JsonWriter* writer, const ledger::ReconcileInfo&);
void saveToJson(JsonWriter* writer, const ledger::RewardsInternalsInfo&);
void saveToJson(JsonWriter* writer, const ledger::WalletProperties&);
void saveToJson(JsonWriter* writer, const WALLET_PROPERTIES_ST&);
void saveToJson(JsonWriter* writer, const GRANT&);

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
