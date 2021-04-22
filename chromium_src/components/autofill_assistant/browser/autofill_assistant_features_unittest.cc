/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "base/feature_list.h"
#include "components/autofill_assistant/browser/features.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace autofill_assistant {

TEST(AutofillAssistantFeaturesTest,
     AutofillAssistantFeedbackChipFeatureDisabled) {
  EXPECT_FALSE(
      base::FeatureList::IsEnabled(features::kAutofillAssistantFeedbackChip));
}

}  // namespace autofill_assistant
