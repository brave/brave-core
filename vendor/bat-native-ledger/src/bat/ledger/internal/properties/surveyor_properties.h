/* Copyright (c) 2019 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

#ifndef BRAVELEDGER_PROPERTIES_SURVEYOR_PROPERTIES_H_
#define BRAVELEDGER_PROPERTIES_SURVEYOR_PROPERTIES_H_

#include <string>

namespace ledger {

struct SurveyorProperties {
  SurveyorProperties();
  SurveyorProperties(
      const SurveyorProperties& properties);
  ~SurveyorProperties();

  bool operator==(
      const SurveyorProperties& rhs) const;

  bool operator!=(
      const SurveyorProperties& rhs) const;

  std::string signature;
  std::string surveyor_id;
  std::string survey_vk;
  std::string registrar_vk;
  std::string survey_sk;
};

}  // namespace ledger

#endif  // BRAVELEDGER_PROPERTIES_SURVEYOR_PROPERTIES_H_
