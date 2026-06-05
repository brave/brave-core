/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_CONTEXTUAL_TEXT_CLASSIFICATION_RESOURCE_TEXT_CLASSIFICATION_PROBABILITIES_DATABASE_TABLE_TEST_UTIL_H_
#define BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_CONTEXTUAL_TEXT_CLASSIFICATION_RESOURCE_TEXT_CLASSIFICATION_PROBABILITIES_DATABASE_TABLE_TEST_UTIL_H_

#include "brave/components/brave_ads/core/internal/targeting/contextual/text_classification/resource/text_classification_probabilities_database_table.h"

namespace brave_ads::test {

void GetAll(
    database::table::GetTextClassificationProbabilitiesCallback callback);

}  // namespace brave_ads::test

#endif  // BRAVE_COMPONENTS_BRAVE_ADS_CORE_INTERNAL_TARGETING_CONTEXTUAL_TEXT_CLASSIFICATION_RESOURCE_TEXT_CLASSIFICATION_PROBABILITIES_DATABASE_TABLE_TEST_UTIL_H_
