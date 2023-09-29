/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/autofill/core/browser/browser_autofill_manager.h"

// replaces OnWillSubmitForm() calls that have 3 arguments
#define OnWillSubmitForm_2(arg1, arg2) OnWillSubmitForm(arg1, arg2)
#define OnWillSubmitForm_3(arg1, arg2, arg3) \
  OnWillSubmitForm(arg1, arg2, client())
#define OnWillSubmitForm_get_overload(first, second, third, overload, ...) \
  overload
#define OnWillSubmitForm(...)                                    \
  OnWillSubmitForm_get_overload(__VA_ARGS__, OnWillSubmitForm_3, \
                                OnWillSubmitForm_2)(__VA_ARGS__)

#include "src/components/autofill/core/browser/browser_autofill_manager.cc"

#undef OnWillSubmitForm
#undef OnWillSubmitForm_get_overload
#undef OnWillSubmitForm_3
#undef OnWillSubmitForm_2
