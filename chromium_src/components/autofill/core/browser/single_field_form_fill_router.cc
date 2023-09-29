/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/autofill/core/browser/single_field_form_fill_router.h"

#define OnWillSubmitForm(form, form_structure, is_autocomplete_enabled) \
  OnWillSubmitForm(form, form_structure, const AutofillClient& client)

// replaces
//   SingleFieldFormFillRouter::OnWillSubmitFormWithFields(
//     const std::vector<FormFieldData>& fields,
//     bool is_autocomplete_enabled
//   )
// with
//   SingleFieldFormFillRouter::OnWillSubmitFormWithFields(
//     const std::vector<FormFieldData>& fields,
//     const AutofillClient& client
//   )
// and also the *_manager_->OnWillSubmitFormWithFields() calls.
#define OnWillSubmitFormWithFields_helper_delete(x)
#define OnWillSubmitFormWithFields_helper_expand(x) x
#define OnWillSubmitFormWithFields_helper_token_bool const AutofillClient& client OnWillSubmitFormWithFields_helper_delete(
#define OnWillSubmitFormWithFields_helper_token_is_autocomplete_enabled OnWillSubmitFormWithFields_helper_expand(client
#define OnWillSubmitFormWithFields(fields, x) \
        OnWillSubmitFormWithFields(fields, OnWillSubmitFormWithFields_helper_token_##x))

#include "src/components/autofill/core/browser/single_field_form_fill_router.cc"

#undef OnWillSubmitFormWithFields
#undef OnWillSubmitFormWithFields_helper_token_is_autocomplete_enabled
#undef OnWillSubmitFormWithFields_helper_token_bool
#undef OnWillSubmitFormWithFields_helper_expand
#undef OnWillSubmitFormWithFields_helper_delete
#undef OnWillSubmitForm
