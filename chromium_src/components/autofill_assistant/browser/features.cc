/* Copyright 2021 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "components/autofill_assistant/browser/features.h"

#define kAutofillAssistantFeedbackChip \
  kAutofillAssistantFeedbackChip_ChromiumImpl

#include "../../../../../components/autofill_assistant/browser/features.cc"
#undef kAutofillAssistantFeedbackChip

namespace autofill_assistant {
namespace features {

const base::Feature kAutofillAssistantFeedbackChip{
    "AutofillAssistantFeedbackChip", base::FEATURE_DISABLED_BY_DEFAULT};

}  // namespace features
}  // namespace autofill_assistant
