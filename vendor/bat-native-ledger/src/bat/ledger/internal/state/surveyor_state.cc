/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/state/surveyor_state.h"
#include "base/json/json_reader.h"
#include "base/logging.h"

namespace ledger {

namespace {

// Do not change these values as they are required to transition legacy state
const char kRegistrarVkKey[] = "registrarVK";
const char kSignatureKey[] = "signature";
const char kSurveyorIdKey[] = "surveyorId";
const char kSurveySkKey[] = "surveySK";
const char kSurveyVkKey[] = "surveyVK";

}  // namespace

SurveyorState::SurveyorState() = default;

SurveyorState::~SurveyorState() = default;

bool SurveyorState::FromJson(
    const std::string& json,
    SurveyorProperties* properties) const {
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

bool SurveyorState::FromDict(
    const base::DictionaryValue* dictionary,
    SurveyorProperties* properties) const {
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

  SurveyorProperties surveyor_properties;

  // Signature
  const auto* signature = dictionary->FindStringKey(kSignatureKey);
  if (!signature) {
    NOTREACHED();
    return false;
  }
  surveyor_properties.signature = *signature;

  // Surveyor Id
  const auto* surveyor_id = dictionary->FindStringKey(kSurveyorIdKey);
  if (!surveyor_id) {
    NOTREACHED();
    return false;
  }
  surveyor_properties.surveyor_id = *surveyor_id;

  // Survey VK
  const auto* survey_vk = dictionary->FindStringKey(kSurveyVkKey);
  if (!survey_vk) {
    NOTREACHED();
    return false;
  }
  surveyor_properties.survey_vk = *survey_vk;

  // Registrar VK
  const auto* registrar_vk = dictionary->FindStringKey(kRegistrarVkKey);
  if (!registrar_vk) {
    NOTREACHED();
    return false;
  }
  surveyor_properties.registrar_vk = *registrar_vk;

  // Survey SK
  const auto* survey_sk = dictionary->FindStringKey(kSurveySkKey);
  if (survey_sk) {
    surveyor_properties.survey_sk = *survey_sk;
  }

  *properties = surveyor_properties;

  return true;
}

bool SurveyorState::ToJson(
    JsonWriter* writer,
    const SurveyorProperties& properties) const {
  DCHECK(writer);
  if (!writer) {
    NOTREACHED();
    return false;
  }

  writer->StartObject();

  writer->String(kRegistrarVkKey);
  writer->String(properties.registrar_vk.c_str());

  writer->String(kSignatureKey);
  writer->String(properties.signature.c_str());

  writer->String(kSurveyorIdKey);
  writer->String(properties.surveyor_id.c_str());

  writer->String(kSurveySkKey);
  writer->String(properties.survey_sk.c_str());

  writer->String(kSurveyVkKey);
  writer->String(properties.survey_vk.c_str());

  writer->EndObject();

  return true;
}

std::string SurveyorState::ToJson(
    const SurveyorProperties& properties) const {
  rapidjson::StringBuffer buffer;
  JsonWriter writer(buffer);

  if (!ToJson(&writer, properties)) {
    NOTREACHED();
    return "";
  }

  return buffer.GetString();
}

}  // namespace ledger
