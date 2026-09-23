/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/autofill/core/browser/foundations/autofill_client.h"

#include <vector>

#include <components/autofill/core/browser/foundations/autofill_client.cc>

namespace autofill {

void AutofillClient::BraveAddSuggestions(
    const PasswordFormClassification& form_classification,
    const FormFieldData& field,
    std::vector<Suggestion>& chromium_suggestions) {}

bool AutofillClient::BraveHandleSuggestion(
    const Suggestion& suggestion,
    const autofill::FieldGlobalId& field) {
  return false;
}

}  // namespace autofill
