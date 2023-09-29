/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_AUTOFILL_CORE_BROWSER_SINGLE_FIELD_FORM_FILLER_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_AUTOFILL_CORE_BROWSER_SINGLE_FIELD_FORM_FILLER_H_

#define OnWillSubmitFormWithFields(fields, is_autocomplete_enabled) \
  OnWillSubmitFormWithFields(fields, const AutofillClient& client)

#include "src/components/autofill/core/browser/single_field_form_filler.h"

#undef OnWillSubmitFormWithFields

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_AUTOFILL_CORE_BROWSER_SINGLE_FIELD_FORM_FILLER_H_
