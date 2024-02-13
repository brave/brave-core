// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#include "components/autofill/core/browser/autofill_field.h"

#define AppendLogEventIfNotRepeated AppendLogEventIfNotRepeated_ChromiumImpl
#include "src/components/autofill/core/browser/autofill_field.cc"
#undef AppendLogEventIfNotRepeated

namespace autofill {

void AutofillField::AppendLogEventIfNotRepeated(
    const FieldLogEventType& log_event) {}

}  // namespace autofill
