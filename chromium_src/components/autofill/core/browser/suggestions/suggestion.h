// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_AUTOFILL_CORE_BROWSER_SUGGESTIONS_SUGGESTION_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_AUTOFILL_CORE_BROWSER_SUGGESTIONS_SUGGESTION_H_

#define IsAcceptable(...)                   \
  brave_new_email_alias_suggestion = false; \
  bool IsAcceptable(__VA_ARGS__)

#include <components/autofill/core/browser/suggestions/suggestion.h>  // IWYU pragma: export

#undef IsAcceptable

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_AUTOFILL_CORE_BROWSER_SUGGESTIONS_SUGGESTION_H_
