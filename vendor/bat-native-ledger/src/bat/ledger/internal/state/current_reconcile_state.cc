/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include <stdint.h>

#include "bat/ledger/internal/state/current_reconcile_state.h"
#include "bat/ledger/internal/state/reconcile_direction_state.h"
#include "base/json/json_reader.h"
#include "base/logging.h"

namespace ledger {

namespace {

// Do not change these values as they are required to transition legacy state
const char kAmountKey[] = "amount";
const char kAnonizeViewingIdKey[] = "anonizeViewingId";
const char kCategoryKey[] = "category";
const char kCurrencyKey[] = "currency";
const char kDestinationKey[] = "destination";
const char kDirectionsKey[] = "directions";
const char kFeeKey[] = "fee";
const char kIdKey[] = "id";
const char kListKey[] = "list";
const char kMasterUserTokenKey[] = "masterUserToken";
const char kPreFlightKey[] = "preFlight";
const char kProofKey[] = "proof";
const char kRatesKey[] = "rates";
const char kRegistrarVkKey[] = "registrarVK";
const char kRetryLevelKey[] = "retry_level";
const char kRetryStepKey[] = "retry_step";
const char kSurveyorIdKey[] = "surveyorId";
const char kSurveyorInfoKey[] = "surveyorInfo";
const char kTimestampKey[] = "timestamp";
const char kTypeKey[] = "type";
const char kViewingIdKey[] = "viewingId";
const char kWeightKey[] = "weight";

}  // namespace

CurrentReconcileState::CurrentReconcileState() = default;

CurrentReconcileState::~CurrentReconcileState() = default;

bool CurrentReconcileState::FromJson(
    const std::string& json,
    CurrentReconcileProperties* properties) const {
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

bool CurrentReconcileState::FromDict(
    const base::DictionaryValue* dictionary,
    CurrentReconcileProperties* properties) const {
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

  CurrentReconcileProperties current_reconcile_properties;

  // Viewing Id
  const auto* viewing_id = dictionary->FindStringKey(kViewingIdKey);
  if (!viewing_id) {
    NOTREACHED();
    return false;
  }
  current_reconcile_properties.viewing_id = *viewing_id;

  // Anonize Viewing Id
  const auto* anonize_viewing_id =
      dictionary->FindStringKey(kAnonizeViewingIdKey);
  if (anonize_viewing_id) {
    current_reconcile_properties.anonize_viewing_id = *anonize_viewing_id;
  }

  // Registrar Vk
  const auto* registrar_vk = dictionary->FindStringKey(kRegistrarVkKey);
  if (registrar_vk) {
    current_reconcile_properties.registrar_vk = *registrar_vk;
  }

  // Pre Flight
  const auto* pre_flight = dictionary->FindStringKey(kPreFlightKey);
  if (pre_flight) {
    current_reconcile_properties.pre_flight = *pre_flight;
  }

  // Master User Token
  const auto* master_user_token =
      dictionary->FindStringKey(kMasterUserTokenKey);
  if (master_user_token) {
    current_reconcile_properties.master_user_token = *master_user_token;
  }

  // Timestamp (There is no support for uint64_t. Writing JSON with such types
  // violates the spec. As we need a uint64_t, we need to use an unsigned int
  // and cast to a uint64_t)
  const auto timestamp = dictionary->FindDoubleKey(kTimestampKey);
  if (timestamp) {
    current_reconcile_properties.timestamp = static_cast<uint64_t>(*timestamp);
  }

  // Amount
  const auto* amount = dictionary->FindStringKey(kAmountKey);
  if (amount) {
    current_reconcile_properties.amount = *amount;
  }

  // Currency
  const auto* currency = dictionary->FindStringKey(kCurrencyKey);
  if (currency) {
    current_reconcile_properties.currency = *currency;
  }

  // Fee
  const auto fee = dictionary->FindDoubleKey(kFeeKey);
  if (!fee) {
    NOTREACHED();
    return false;
  }
  current_reconcile_properties.fee = *fee;

  // Type
  const auto type = dictionary->FindIntKey(kTypeKey);
  if (type) {
    current_reconcile_properties.type = static_cast<RewardsType>(*type);
  } else {
    const auto category = dictionary->FindIntKey(kCategoryKey);
    if (!category) {
      NOTREACHED();
      return false;
    }

    // Transition from legacy Category to Type
    current_reconcile_properties.type = static_cast<RewardsType>(*category);
  }

  // Surveyor Info
  const auto* surveyor_info_dictionary =
      dictionary->FindDictKey(kSurveyorInfoKey);
  if (surveyor_info_dictionary) {
    const auto* surveyor_id =
        surveyor_info_dictionary->FindStringKey(kSurveyorIdKey);
    if (!surveyor_id) {
      NOTREACHED();
      return false;
    }

    current_reconcile_properties.surveyor_id = *surveyor_id;
  }

  // Rates
  const auto* rates = dictionary->FindDictKey(kRatesKey);
  if (rates) {
    for (const auto& rate : rates->DictItems()) {
      const std::string currency_code = rate.first;

      const auto& exchange_rate_value = rate.second;
      const double exchange_rate = exchange_rate_value.GetDouble();

      current_reconcile_properties.rates.insert({currency_code, exchange_rate});
    }
  }

  // Reconcile Directions
  const auto* reconcile_directions_list =
      dictionary->FindListKey(kDirectionsKey);
  if (reconcile_directions_list) {
    const ReconcileDirectionState reconcile_direction_state;
    for (const auto& reconcile_direction_value :
        reconcile_directions_list->GetList()) {
      if (!reconcile_direction_value.is_dict()) {
        NOTREACHED();
        continue;
      }

      const base::DictionaryValue* reconcile_direction_dictionary = nullptr;
      reconcile_direction_value.GetAsDictionary(
          &reconcile_direction_dictionary);
      if (!reconcile_direction_dictionary) {
        NOTREACHED();
        continue;
      }

      ReconcileDirectionProperties reconcile_direction;
      if (!reconcile_direction_state.FromDict(reconcile_direction_dictionary,
          &reconcile_direction)) {
        continue;
      }

      current_reconcile_properties.directions.push_back(reconcile_direction);
    }
  }

  // Transition legacy list to reconcile directions
  const auto* list_list = dictionary->FindListKey(kListKey);
  if (list_list) {
    for (const auto& list_value : list_list->GetList()) {
      if (!list_value.is_dict()) {
        NOTREACHED();
        continue;
      }

      const base::DictionaryValue* list_dictionary = nullptr;
      list_value.GetAsDictionary(&list_dictionary);
      if (!list_dictionary) {
        NOTREACHED();
        continue;
      }

      ReconcileDirectionProperties reconcile_direction;

      const auto* id = list_dictionary->FindStringKey(kIdKey);
      if (!id) {
        NOTREACHED();
        continue;
      }
      reconcile_direction.publisher_key = *id;

      const auto weight = list_dictionary->FindDoubleKey(kWeightKey);
      if (!weight) {
        NOTREACHED();
        continue;
      }
      reconcile_direction.amount_percent = *weight;

      current_reconcile_properties.directions.push_back(reconcile_direction);
    }
  }

  // Retry Step
  const auto retry_step = dictionary->FindIntKey(kRetryStepKey);
  if (retry_step) {
    current_reconcile_properties.retry_step =
        static_cast<ContributionRetry>(*retry_step);
  }

  // Retry Level
  const auto retry_level = dictionary->FindIntKey(kRetryLevelKey);
  if (retry_level) {
    current_reconcile_properties.retry_level = *retry_level;
  }

  // Destination
  const auto* destination = dictionary->FindStringKey(kDestinationKey);
  if (destination) {
    current_reconcile_properties.destination = *destination;
  }

  // Proof
  const auto* proof = dictionary->FindStringKey(kProofKey);
  if (proof) {
    current_reconcile_properties.proof = *proof;
  }

  *properties = current_reconcile_properties;

  return true;
}

bool CurrentReconcileState::ToJson(
    JsonWriter* writer,
    const CurrentReconcileProperties& properties) const {
  DCHECK(writer);
  if (!writer) {
    NOTREACHED();
    return false;
  }

  writer->StartObject();

  writer->String(kViewingIdKey);
  writer->String(properties.viewing_id.c_str());

  writer->String(kAnonizeViewingIdKey);
  writer->String(properties.anonize_viewing_id.c_str());

  writer->String(kRegistrarVkKey);
  writer->String(properties.registrar_vk.c_str());

  writer->String(kPreFlightKey);
  writer->String(properties.pre_flight.c_str());

  writer->String(kMasterUserTokenKey);
  writer->String(properties.master_user_token.c_str());

  writer->String(kSurveyorInfoKey);
  writer->StartObject();
  writer->String(kSurveyorIdKey);
  writer->String(properties.surveyor_id.c_str());
  writer->EndObject();

  writer->String(kTimestampKey);
  writer->Uint64(properties.timestamp);

  writer->String(kAmountKey);
  writer->String(properties.amount.c_str());

  writer->String(kCurrencyKey);
  writer->String(properties.currency.c_str());

  writer->String(kFeeKey);
  writer->Double(properties.fee);

  writer->String(kTypeKey);
  writer->Int(static_cast<int>(properties.type));

  writer->String(kRatesKey);
  writer->StartObject();
  for (const auto &rate : properties.rates) {
    writer->String(rate.first.c_str());
    writer->Double(rate.second);
  }
  writer->EndObject();

  writer->String(kDirectionsKey);
  writer->StartArray();
  const ReconcileDirectionState reconcile_direction_state;
  for (const auto& direction : properties.directions) {
    reconcile_direction_state.ToJson(writer, direction);
  }
  writer->EndArray();

  writer->String(kRetryStepKey);
  writer->Int(static_cast<int32_t>(properties.retry_step));

  writer->String(kRetryLevelKey);
  writer->Int(properties.retry_level);

  writer->String(kDestinationKey);
  writer->String(properties.destination.c_str());

  writer->String(kProofKey);
  writer->String(properties.proof.c_str());

  writer->EndObject();

  return true;
}

std::string CurrentReconcileState::ToJson(
    const CurrentReconcileProperties& properties) const {
  rapidjson::StringBuffer buffer;
  JsonWriter writer(buffer);

  if (!ToJson(&writer, properties)) {
    NOTREACHED();
    return "";
  }

  return buffer.GetString();
}

}  // namespace ledger
