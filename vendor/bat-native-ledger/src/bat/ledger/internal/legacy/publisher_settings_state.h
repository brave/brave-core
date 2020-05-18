/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_LEGACY_PUBLISHER_SETTINGS_STATE_H_
#define BRAVELEDGER_LEGACY_PUBLISHER_SETTINGS_STATE_H_

#include <string>

#include "bat/ledger/internal/legacy/state_reader.h"
#include "bat/ledger/internal/legacy/state_writer.h"
#include "bat/ledger/internal/legacy/publisher_settings_properties.h"
#include "rapidjson/writer.h"

namespace ledger {

using JsonWriter = rapidjson::Writer<rapidjson::StringBuffer>;

class PublisherSettingsState
    : public state::Reader<PublisherSettingsProperties>,
      public state::Writer<JsonWriter*, PublisherSettingsProperties> {
 public:
  PublisherSettingsState();
  ~PublisherSettingsState();

  bool FromJson(
      const std::string& json,
      PublisherSettingsProperties* properties) const override;

  bool FromDict(
      const base::DictionaryValue* dictionary,
      PublisherSettingsProperties* properties) const override;

  bool ToJson(
      JsonWriter* writer,
      const PublisherSettingsProperties& properties) const override;

  std::string ToJson(
      const PublisherSettingsProperties& properties) const override;
};

}  // namespace ledger

#endif  // BRAVELEDGER_LEGACY_PUBLISHER_SETTINGS_STATE_H_
