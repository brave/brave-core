/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/state/ballot_state.h"
#include "base/json/json_reader.h"
#include "base/logging.h"

namespace ledger {

namespace {

// Do not change these values as they are required to transition legacy state
const char kCountKey[] = "offset";
const char kPrepareBallotKey[] = "prepareBallot";
const char kPublisherKey[] = "publisher";
const char kSurveyorIdKey[] = "surveyorId";
const char kViewingIdKey[] = "viewingId";

}  // namespace

BallotState::BallotState() = default;

BallotState::~BallotState() = default;

bool BallotState::FromJson(
    const std::string& json,
    BallotProperties* properties) const {
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

bool BallotState::FromDict(
    const base::DictionaryValue* dictionary,
    BallotProperties* properties) const {
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

  BallotProperties ballot_properties;

  // Viewing Id
  const auto* viewing_id = dictionary->FindStringKey(kViewingIdKey);
  if (!viewing_id) {
    NOTREACHED();
    return false;
  }
  ballot_properties.viewing_id = *viewing_id;

  // Surveyor Id
  const auto* surveyor_id = dictionary->FindStringKey(kSurveyorIdKey);
  if (!surveyor_id) {
    NOTREACHED();
    return false;
  }
  ballot_properties.surveyor_id = *surveyor_id;

  // Publisher
  const auto* publisher = dictionary->FindStringKey(kPublisherKey);
  if (!publisher) {
    NOTREACHED();
    return false;
  }
  ballot_properties.publisher = *publisher;

  // Offset (There is no support for unsigned int. Writing JSON with such types
  // violates the spec. As we need an unsigned int, we need to a double and cast
  // to an unsigned int)
  const auto count = dictionary->FindDoubleKey(kCountKey);
  if (!count) {
    NOTREACHED();
    return false;
  }
  ballot_properties.count = static_cast<unsigned int>(*count);

  // Prepare Ballot
  const auto* prepare_ballot = dictionary->FindStringKey(kPrepareBallotKey);
  if (!prepare_ballot) {
    NOTREACHED();
    return false;
  }
  ballot_properties.prepare_ballot = *prepare_ballot;

  *properties = ballot_properties;

  return true;
}

bool BallotState::ToJson(
    JsonWriter* writer,
    const BallotProperties& properties) const {
  DCHECK(writer);
  if (!writer) {
    NOTREACHED();
    return false;
  }

  writer->StartObject();

  writer->String(kViewingIdKey);
  writer->String(properties.viewing_id.c_str());

  writer->String(kSurveyorIdKey);
  writer->String(properties.surveyor_id.c_str());

  writer->String(kPublisherKey);
  writer->String(properties.publisher.c_str());

  writer->String(kCountKey);
  writer->Uint(properties.count);

  writer->String(kPrepareBallotKey);
  writer->String(properties.prepare_ballot.c_str());

  writer->EndObject();

  return true;
}

std::string BallotState::ToJson(
    const BallotProperties& properties) const {
  rapidjson::StringBuffer buffer;
  JsonWriter writer(buffer);

  if (!ToJson(&writer, properties)) {
    NOTREACHED();
    return "";
  }

  return buffer.GetString();
}

}  // namespace ledger
