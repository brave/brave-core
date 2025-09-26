/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/autofill/content/browser/content_autofill_client.h"

#include <components/autofill/content/browser/content_autofill_client.cc>

namespace autofill {

AutofillOptimizationGuideDecider*
BraveContentAutofillClientUnused::GetAutofillOptimizationGuideDecider_Unused()
    const {
  return nullptr;
}

bool BraveContentAutofillClientUnused::IsAutofillEnabled_Unused() const {
  return false;
}

bool BraveContentAutofillClientUnused::IsAutocompleteEnabled_Unused() const {
  return false;
}

}  // namespace autofill
