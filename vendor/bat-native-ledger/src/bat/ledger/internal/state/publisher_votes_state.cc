/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/state/publisher_votes_state.h"
#include "bat/ledger/internal/state/publisher_vote_state.h"
#include "base/json/json_reader.h"
#include "base/logging.h"

namespace ledger {

namespace {

// Do not change these values as they are required to transition legacy state
const char kPublisherKey[] = "publisher";
const char kBatchVotesKey[] = "batchVotesInfo";

}  // namespace

PublisherVotesState::PublisherVotesState() = default;

PublisherVotesState::~PublisherVotesState() = default;

bool PublisherVotesState::FromJson(
    const std::string& json,
    PublisherVotesProperties* properties) const {
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

bool PublisherVotesState::FromDict(
    const base::DictionaryValue* dictionary,
    PublisherVotesProperties* properties) const {
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

  PublisherVotesProperties publisher_votes_properties;

  // Publisher
  const auto* publisher = dictionary->FindStringKey(kPublisherKey);
  if (!publisher) {
    NOTREACHED();
    return false;
  }
  publisher_votes_properties.publisher = *publisher;

  // Batch Votes
  const auto* batch_votes_list = dictionary->FindListKey(kBatchVotesKey);
  if (!batch_votes_list) {
    NOTREACHED();
    return false;
  }

  const PublisherVoteState publisher_vote_state;
  for (const auto& batch_votes_value : batch_votes_list->GetList()) {
    if (!batch_votes_value.is_dict()) {
      NOTREACHED();
      continue;
    }

    const base::DictionaryValue* batch_votes_dictionary = nullptr;
    batch_votes_value.GetAsDictionary(&batch_votes_dictionary);
    if (!batch_votes_dictionary) {
      NOTREACHED();
      continue;
    }

    PublisherVoteProperties publisher_vote_properties;
    if (!publisher_vote_state.FromDict(batch_votes_dictionary,
        &publisher_vote_properties)) {
      continue;
    }

    publisher_votes_properties.batch_votes.push_back(
        publisher_vote_properties);
  }

  *properties = publisher_votes_properties;

  return true;
}

bool PublisherVotesState::ToJson(
    JsonWriter* writer,
    const PublisherVotesProperties& properties) const {
  DCHECK(writer);
  if (!writer) {
    NOTREACHED();
    return false;
  }

  writer->StartObject();

  writer->String(kPublisherKey);
  writer->String(properties.publisher.c_str());

  writer->String(kBatchVotesKey);
  writer->StartArray();
  const PublisherVoteState publisher_vote_state;
  for (const auto& batch_vote : properties.batch_votes) {
    publisher_vote_state.ToJson(writer, batch_vote);
  }
  writer->EndArray();

  writer->EndObject();

  return true;
}

std::string PublisherVotesState::ToJson(
    const PublisherVotesProperties& properties) const {
  rapidjson::StringBuffer buffer;
  JsonWriter writer(buffer);

  if (!ToJson(&writer, properties)) {
    NOTREACHED();
    return "";
  }

  return buffer.GetString();
}

}  // namespace ledger
