/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_STATE_PUBLISHER_VOTES_STATE_H_
#define BRAVELEDGER_STATE_PUBLISHER_VOTES_STATE_H_

#include <string>

#include "bat/ledger/internal/state/state_reader.h"
#include "bat/ledger/internal/state/state_writer.h"
#include "bat/ledger/internal/properties/publisher_votes_properties.h"
#include "rapidjson/writer.h"

namespace ledger {

using JsonWriter = rapidjson::Writer<rapidjson::StringBuffer>;

class PublisherVotesState
    : public state::Reader<PublisherVotesProperties>,
      public state::Writer<JsonWriter*, PublisherVotesProperties> {
 public:
  PublisherVotesState();
  ~PublisherVotesState();

  bool FromJson(
      const std::string& json,
      PublisherVotesProperties* properties) const override;

  bool FromDict(
      const base::DictionaryValue* dictionary,
      PublisherVotesProperties* properties) const override;

  bool ToJson(
      JsonWriter* writer,
      const PublisherVotesProperties& properties) const override;

  std::string ToJson(
      const PublisherVotesProperties& properties) const override;
};

}  // namespace ledger

#endif  // BRAVELEDGER_STATE_PUBLISHER_VOTES_STATE_H_
