/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_STATE_SURVEYOR_STATE_H_
#define BRAVELEDGER_STATE_SURVEYOR_STATE_H_

#include <string>

#include "bat/ledger/internal/state/state_reader.h"
#include "bat/ledger/internal/state/state_writer.h"
#include "bat/ledger/internal/properties/surveyor_properties.h"
#include "rapidjson/writer.h"

namespace ledger {

using JsonWriter = rapidjson::Writer<rapidjson::StringBuffer>;

class SurveyorState
    : public state::Reader<SurveyorProperties>,
      public state::Writer<JsonWriter*, SurveyorProperties> {
 public:
  SurveyorState();
  ~SurveyorState();

  bool FromJson(
      const std::string& json,
      SurveyorProperties* properties) const override;

  bool FromDict(
      const base::DictionaryValue* dictionary,
      SurveyorProperties* properties) const override;

  bool ToJson(
      JsonWriter* writer,
      const SurveyorProperties& properties) const override;

  std::string ToJson(
      const SurveyorProperties& properties) const override;
};

}  // namespace ledger

#endif  // BRAVELEDGER_STATE_SURVEYOR_STATE_H_
