/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/state/publisher_vote_state.h"
#include "base/json/json_reader.h"
#include "base/logging.h"

namespace ledger {

namespace {

// Do not change these values as they are required to transition legacy state
const char kProofKey[] = "proof";
const char kSurveyorIdKey[] = "surveyorId";

}  // namespace

PublisherVoteState::PublisherVoteState() = default;

PublisherVoteState::~PublisherVoteState() = default;

bool PublisherVoteState::FromJson(
    const std::string& json,
    PublisherVoteProperties* properties) const {
  DCHECK(properties);
  if (!properties) {
    NOTREACHED();
    return false;
  }

  auto json_value = base::JSONReader::Read(json);
  if (!json_value) {
    NOTREACHED();
    return false;
  }

  base::DictionaryValue* dictionary = nullptr;
  json_value->GetAsDictionary(&dictionary);
  if (!dictionary) {
    NOTREACHED();
    return false;
  }

  return FromDict(dictionary, properties);
}

bool PublisherVoteState::FromDict(
    const base::DictionaryValue* dictionary,
    PublisherVoteProperties* properties) const {
  DCHECK(dictionary);
  if (!dictionary) {
    NOTREACHED();
    return false;
  }

  DCHECK(properties);
  if (!properties) {
    NOTREACHED();
    return false;
  }

  PublisherVoteProperties vote_properties;

  // Surveyor Id
  const auto* surveyor_id = dictionary->FindStringKey(kSurveyorIdKey);
  if (!surveyor_id) {
    NOTREACHED();
    return false;
  }
  vote_properties.surveyor_id = *surveyor_id;

  // Proof
  const auto* proof = dictionary->FindStringKey(kProofKey);
  if (!proof) {
    NOTREACHED();
    return false;
  }
  vote_properties.proof = *proof;

  *properties = vote_properties;

  return true;
}

bool PublisherVoteState::ToJson(
    JsonWriter* writer,
    const PublisherVoteProperties& properties) const {
  DCHECK(writer);
  if (!writer) {
    NOTREACHED();
    return false;
  }

  writer->StartObject();

  writer->String("surveyorId");
  writer->String(properties.surveyor_id.c_str());

  writer->String("proof");
  writer->String(properties.proof.c_str());

  writer->EndObject();

  return true;
}

std::string PublisherVoteState::ToJson(
    const PublisherVoteProperties& properties) const {
  rapidjson::StringBuffer buffer;
  JsonWriter writer(buffer);

  if (!ToJson(&writer, properties)) {
    NOTREACHED();
    return "";
  }

  return buffer.GetString();
}

std::string PublisherVoteState::ToJson(
    const BatchVotes& batch_votes) const {
  rapidjson::StringBuffer buffer;
  JsonWriter writer(buffer);

  writer.StartArray();
  for (const auto& batch_vote : batch_votes) {
    if (!ToJson(&writer, batch_vote)) {
      NOTREACHED();
      continue;
    }
  }
  writer.EndArray();

  return buffer.GetString();
}

}  // namespace ledger
