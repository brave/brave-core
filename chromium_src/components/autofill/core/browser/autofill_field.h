// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_AUTOFILL_CORE_BROWSER_AUTOFILL_FIELD_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_AUTOFILL_CORE_BROWSER_AUTOFILL_FIELD_H_

#define AppendLogEventIfNotRepeated         \
  AppendLogEventIfNotRepeated_ChromiumImpl( \
      const FieldLogEventType& log_event);  \
  void AppendLogEventIfNotRepeated

#include "src/components/autofill/core/browser/autofill_field.h"  // IWYU pragma: export
#undef AppendLogEventIfNotRepeated

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_AUTOFILL_CORE_BROWSER_AUTOFILL_FIELD_H_
