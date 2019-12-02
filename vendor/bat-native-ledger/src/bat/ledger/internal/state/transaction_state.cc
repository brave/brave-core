/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/state/transaction_ballot_state.h"
#include "bat/ledger/internal/state/transaction_state.h"
#include "base/json/json_reader.h"
#include "base/logging.h"

namespace ledger {

namespace {

// Do not change these values as they are required to transition legacy state
const char kAnonizeViewingIdKey[] = "anonizeViewingId";
const char kBallotsKey[] = "ballots";
const char kContributionProbiKey[] = "contribution_probi";
const char kContributionRatesKey[] = "rates";
const char kMasterUserTokenKey[] = "masterUserToken";
const char kRegistrarVkKey[] = "registrarVK";
const char kSubmissionTimestampKey[] = "submissionStamp";
const char kSurveyorIdKey[] = "surveyorId";
const char kSurveyorIdsKey[] = "surveyorIds";
const char kViewingIdKey[] = "viewingId";
const char kVoteCountKey[] = "votes";

}  // namespace

TransactionState::TransactionState() = default;

TransactionState::~TransactionState() = default;

bool TransactionState::FromJson(
    const std::string& json,
    TransactionProperties* properties) const {
  DCHECK(properties);
  if (!properties) {
    NOTREACHED();
    return false;
  }

  auto json_value = base::JSONReader::Read(json);
  if (!json_value) {
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

bool TransactionState::FromJsonResponse(
    const std::string& json,
    TransactionProperties* properties) const {
  DCHECK(properties);
  if (!properties) {
    NOTREACHED();
    return false;
  }

  auto json_value = base::JSONReader::Read(json);
  if (!json_value) {
    return false;
  }

  base::DictionaryValue* dictionary = nullptr;
  json_value->GetAsDictionary(&dictionary);
  if (!dictionary) {
    NOTREACHED();
    return false;
  }

  TransactionProperties transaction_properties;

  // Contribution Probi
  const auto* contribution_probi =
      dictionary->FindStringKey(kContributionProbiKey);
  if (!contribution_probi) {
    NOTREACHED();
    return false;
  }
  transaction_properties.contribution_probi = *contribution_probi;

  // Submission Timestamp
  const auto* submission_timestamp =
      dictionary->FindStringKey(kSubmissionTimestampKey);
  if (!submission_timestamp) {
    NOTREACHED();
    return false;
  }
  transaction_properties.submission_timestamp = *submission_timestamp;

  *properties = transaction_properties;

  return true;
}

bool TransactionState::FromDict(
    const base::DictionaryValue* dictionary,
    TransactionProperties* properties) const {
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

  TransactionProperties transaction;

  // Viewing Id
  const auto* viewing_id = dictionary->FindStringKey(kViewingIdKey);
  if (!viewing_id) {
    NOTREACHED();
    return false;
  }
  transaction.viewing_id = *viewing_id;

  // Surveyor Id
  const auto* surveyor_id = dictionary->FindStringKey(kSurveyorIdKey);
  if (!surveyor_id) {
    NOTREACHED();
    return false;
  }
  transaction.surveyor_id = *surveyor_id;

  // Contribution Rates
  const auto* contributions_rates =
      dictionary->FindDictKey(kContributionRatesKey);
  if (!contributions_rates) {
    NOTREACHED();
    return false;
  }

  for (const auto& contribution_rate : contributions_rates->DictItems()) {
    const std::string currency_code = contribution_rate.first;

    const auto& exchange_rate_value = contribution_rate.second;
    const double exchange_rate = exchange_rate_value.GetDouble();

    transaction.contribution_rates.insert(
        {currency_code, exchange_rate});
  }

  // Contribution Probi
  const auto* contribution_probi =
      dictionary->FindStringKey(kContributionProbiKey);
  if (!contribution_probi) {
    NOTREACHED();
    return false;
  }
  transaction.contribution_probi = *contribution_probi;

  // Submission Timestamp
  const auto* submission_timestamp =
      dictionary->FindStringKey(kSubmissionTimestampKey);
  if (!submission_timestamp) {
    NOTREACHED();
    return false;
  }
  transaction.submission_timestamp = *submission_timestamp;

  // Anonize Viewing Id
  const auto* anonize_viewing_id =
      dictionary->FindStringKey(kAnonizeViewingIdKey);
  if (!anonize_viewing_id) {
    NOTREACHED();
    return false;
  }
  transaction.anonize_viewing_id = *anonize_viewing_id;

  // Registrar VK
  const auto* registrar_vk = dictionary->FindStringKey(kRegistrarVkKey);
  if (!registrar_vk) {
    NOTREACHED();
    return false;
  }
  transaction.registrar_vk = *registrar_vk;

  // Master user token
  const auto* master_user_token =
      dictionary->FindStringKey(kMasterUserTokenKey);
  if (!master_user_token) {
    NOTREACHED();
    return false;
  }
  transaction.master_user_token = *master_user_token;

  // Surveyor Ids
  const auto* surveyor_ids_list = dictionary->FindListKey(kSurveyorIdsKey);
  if (!surveyor_ids_list) {
    NOTREACHED();
    return false;
  }

  for (const auto& surveyor_id_value : surveyor_ids_list->GetList()) {
    if (!surveyor_id_value.is_string()) {
      NOTREACHED();
      continue;
    }

    const std::string surveyor_id = surveyor_id_value.GetString();

    transaction.surveyor_ids.push_back(surveyor_id);
  }

  // Votes (There is no support for unsigned int. Writing JSON with such types
  // violates the spec. As we need an unsigned int, we need to a double and cast
  // to an unsigned int)
  const auto vote_count = dictionary->FindDoubleKey(kVoteCountKey);
  if (!vote_count) {
    NOTREACHED();
    return false;
  }
  transaction.vote_count = static_cast<unsigned int>(*vote_count);

  // Ballots
  const auto* ballots_list = dictionary->FindListKey(kBallotsKey);
  if (!ballots_list) {
    NOTREACHED();
    return false;
  }

  const TransactionBallotState transaction_ballance_state;
  for (const auto& ballot_value : ballots_list->GetList()) {
    if (!ballot_value.is_dict()) {
      NOTREACHED();
      continue;
    }

    const base::DictionaryValue* ballot_dictionary = nullptr;
    ballot_value.GetAsDictionary(&ballot_dictionary);
    if (!ballot_dictionary) {
      NOTREACHED();
      continue;
    }

    TransactionBallotProperties transaction_ballot;
    if (!transaction_ballance_state.FromDict(ballot_dictionary,
        &transaction_ballot)) {
      continue;
    }

    transaction.transaction_ballots.push_back(transaction_ballot);
  }

  *properties = transaction;

  return true;
}

bool TransactionState::ToJson(
    JsonWriter* writer,
    const TransactionProperties& properties) const {
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

  writer->String(kContributionRatesKey);
  writer->StartObject();
  for (const auto& contribution_rate : properties.contribution_rates) {
    writer->String(contribution_rate.first.c_str());
    writer->Double(contribution_rate.second);
  }
  writer->EndObject();

  writer->String(kContributionProbiKey);
  writer->String(properties.contribution_probi.c_str());

  writer->String(kSubmissionTimestampKey);
  writer->String(properties.submission_timestamp.c_str());

  writer->String(kAnonizeViewingIdKey);
  writer->String(properties.anonize_viewing_id.c_str());

  writer->String(kRegistrarVkKey);
  writer->String(properties.registrar_vk.c_str());

  writer->String(kMasterUserTokenKey);
  writer->String(properties.master_user_token.c_str());

  writer->String(kSurveyorIdsKey);
  writer->StartArray();
  for (const auto& surveyor_id : properties.surveyor_ids) {
    writer->String(surveyor_id.c_str());
  }
  writer->EndArray();

  writer->String(kVoteCountKey);
  writer->Uint(properties.vote_count);

  writer->String(kBallotsKey);
  writer->StartArray();
  const TransactionBallotState transaction_ballot_state;
  for (const auto& ballot : properties.transaction_ballots) {
    transaction_ballot_state.ToJson(writer, ballot);
  }
  writer->EndArray();

  writer->EndObject();

  return true;
}

std::string TransactionState::ToJson(
    const TransactionProperties& properties) const {
  rapidjson::StringBuffer buffer;
  JsonWriter writer(buffer);

  if (!ToJson(&writer, properties)) {
    NOTREACHED();
    return "";
  }

  return buffer.GetString();
}

}  // namespace ledger
