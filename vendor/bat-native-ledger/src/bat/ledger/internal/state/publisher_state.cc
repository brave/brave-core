/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/state/publisher_state.h"
#include "bat/ledger/mojom_structs.h"
#include "base/json/json_reader.h"
#include "base/logging.h"

namespace ledger {

namespace {

// Do not change these values as they are required to transition legacy state
const char kDurationKey[] = "duration";
const char kIdKey[] = "id";
const char kPercentKey[] = "percent";
const char kScoreKey[] = "score";
const char kStatusKey[] = "status";
const char kVerifiedKey[] = "verified";
const char kVisitsKey[] = "visits";
const char kWeightKey[] = "weight";

}  // namespace

PublisherState::PublisherState() = default;

PublisherState::~PublisherState() = default;

bool PublisherState::FromJson(
    const std::string& json,
    PublisherProperties* properties) const {
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

bool PublisherState::FromDict(
    const base::DictionaryValue* dictionary,
    PublisherProperties* properties) const {
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

  PublisherProperties publisher_properties;

  // Id
  const auto* id = dictionary->FindStringKey(kIdKey);
  if (!id) {
    NOTREACHED();
    return false;
  }
  publisher_properties.id = *id;

  // Duration (There is no support for uint64_t. Writing JSON with such types
  // violates the spec. As we need a uint64_t, we need to use an unsigned int
  // and cast to a uint64_t)
  const auto duration = dictionary->FindDoubleKey(kDurationKey);
  if (!duration) {
    NOTREACHED();
    return false;
  }
  publisher_properties.duration = static_cast<uint64_t>(*duration);

  // Score
  auto score = dictionary->FindDoubleKey(kScoreKey);
  if (!score) {
    NOTREACHED();
    return false;
  }
  publisher_properties.score = *score;

  // Visits (There is no support for unsigned int. Writing JSON with such types
  // violates the spec. As we need an unsigned int, we need to a double and cast
  // to an unsigned int)
  const auto visits = dictionary->FindDoubleKey(kVisitsKey);
  if (!visits) {
    NOTREACHED();
    return false;
  }
  publisher_properties.visits = static_cast<unsigned int>(*visits);

  // Percent (There is no support for unsigned int. Writing JSON with such types
  // violates the spec. As we need an unsigned int, we need to a double and cast
  // to an unsigned int)
  const auto percent = dictionary->FindDoubleKey(kPercentKey);
  if (!percent) {
    NOTREACHED();
    return false;
  }
  publisher_properties.percent = static_cast<unsigned int>(*percent);

  // Weight
  const auto weight = dictionary->FindDoubleKey(kWeightKey);
  if (!weight) {
    NOTREACHED();
    return false;
  }
  publisher_properties.weight = *weight;

  // Verified
  const auto verified = dictionary->FindBoolKey(kVerifiedKey);
  if (!verified) {
    // Status (There is no support for unsigned int. Writing JSON with such
    // types violates the spec. As we need an unsigned int, we need to a double
    // and cast to an unsigned int)
    const auto status = dictionary->FindDoubleKey(kStatusKey);
    if (!status) {
      NOTREACHED();
      return false;
    }

    publisher_properties.status = static_cast<unsigned int>(*status);
  } else {
    // Transition legacy verified flag to PublisherStatus enum
    if (*verified) {
      publisher_properties.status = static_cast<unsigned int>(
          PublisherStatus::VERIFIED);
    } else {
      publisher_properties.status = static_cast<unsigned int>(
          PublisherStatus::NOT_VERIFIED);
    }
  }

  *properties = publisher_properties;

  return true;
}

bool PublisherState::ToJson(
    JsonWriter* writer,
    const PublisherProperties& properties) const {
  DCHECK(writer);
  if (!writer) {
    NOTREACHED();
    return false;
  }

  writer->StartObject();

  writer->String(kIdKey);
  writer->String(properties.id.c_str());

  writer->String(kDurationKey);
  writer->Uint64(properties.duration);

  writer->String(kScoreKey);
  writer->Double(properties.score);

  writer->String(kVisitsKey);
  writer->Uint(properties.visits);

  writer->String(kPercentKey);
  writer->Uint(properties.percent);

  writer->String(kWeightKey);
  writer->Double(properties.weight);

  writer->String(kStatusKey);
  writer->Uint(properties.status);

  writer->EndObject();

  return true;
}

std::string PublisherState::ToJson(
    const PublisherProperties& properties) const {
  rapidjson::StringBuffer buffer;
  JsonWriter writer(buffer);

  if (!ToJson(&writer, properties)) {
    NOTREACHED();
    return "";
  }

  return buffer.GetString();
}

}  // namespace ledger
