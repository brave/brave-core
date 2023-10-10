/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/autofill/core/browser/merchant_promo_code_manager.h"
#include "components/autofill/core/browser/single_field_form_fill_router.h"

#define OnWillSubmitFormWithFields(fields, is_autocomplete_enabled) \
        OnWillSubmitFormWithFields(fields, const AutofillClient& client)
#include "src/components/autofill/core/browser/merchant_promo_code_manager.cc"

#undef OnWillSubmitFormWithFields
