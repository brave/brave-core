/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_AUTOFILL_CORE_BROWSER_AUTOCOMPLETE_HISTORY_MANAGER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_AUTOFILL_CORE_BROWSER_AUTOCOMPLETE_HISTORY_MANAGER_H_

#include "components/autofill/core/browser/autofill_client.h"
#include "components/autofill/core/browser/single_field_form_filler.h"

#define OnWillSubmitFormWithFields(fields, is_autocomplete_enabled) \
        OnWillSubmitFormWithFields(fields, const AutofillClient& client)

#define OnAutofillCleanupReturned                            \
  OnConfirmAutocomplete(                                     \
    std::vector<FormFieldData> autocomplete_saveable_fields, \
    absl::optional<bool> user_decision);                     \
  void OnAutofillCleanupReturned

#include "src/components/autofill/core/browser/autocomplete_history_manager.h"
#undef OnAutofillCleanupReturned
#undef OnWillSubmitFormWithFields

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_AUTOFILL_CORE_BROWSER_AUTOCOMPLETE_HISTORY_MANAGER_H_
