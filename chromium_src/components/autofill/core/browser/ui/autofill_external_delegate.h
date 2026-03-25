// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_AUTOFILL_CORE_BROWSER_UI_AUTOFILL_EXTERNAL_DELEGATE_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_AUTOFILL_CORE_BROWSER_UI_AUTOFILL_EXTERNAL_DELEGATE_H_

#include "components/autofill/core/browser/ui/autofill_suggestion_delegate.h"

#define DidAcceptSuggestion(...)                 \
  DidAcceptSuggestion_ChromiumImpl(__VA_ARGS__); \
  void DidAcceptSuggestion(__VA_ARGS__)

#include <components/autofill/core/browser/ui/autofill_external_delegate.h>  // IWYU pragma: export

#undef DidAcceptSuggestion

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_AUTOFILL_CORE_BROWSER_UI_AUTOFILL_EXTERNAL_DELEGATE_H_
