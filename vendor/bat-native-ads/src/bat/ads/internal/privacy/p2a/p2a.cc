/* Copyright (c) 2020 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "bat/ads/internal/privacy/p2a/p2a.h"

#include "bat/ads/internal/ads_client_helper.h"
#include "bat/ads/internal/privacy/p2a/p2a_value_util.h"

namespace ads::privacy::p2a {

void RecordEvent(const std::string& name,
                 const std::vector<std::string>& questions) {
  DCHECK(!name.empty());
  DCHECK(!questions.empty());

  AdsClientHelper::GetInstance()->RecordP2AEvent(name,
                                                 QuestionsToValue(questions));
}

}  // namespace ads::privacy::p2a
