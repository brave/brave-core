/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "bat/ledger/internal/properties/surveyor_properties.h"

namespace ledger {

SurveyorProperties::SurveyorProperties() = default;

SurveyorProperties::SurveyorProperties(
    const SurveyorProperties& properties) {
  signature = properties.signature;
  surveyor_id = properties.surveyor_id;
  survey_vk = properties.survey_vk;
  registrar_vk = properties.registrar_vk;
  survey_sk = properties.survey_sk;
}

SurveyorProperties::~SurveyorProperties() = default;

bool SurveyorProperties::operator==(
    const SurveyorProperties& rhs) const {
  return signature == rhs.signature &&
      surveyor_id == rhs.surveyor_id &&
      survey_vk == rhs.survey_vk &&
      registrar_vk == rhs.registrar_vk &&
      survey_sk == rhs.survey_sk;
}

bool SurveyorProperties::operator!=(
    const SurveyorProperties& rhs) const {
  return !(*this == rhs);
}

}  // namespace ledger
