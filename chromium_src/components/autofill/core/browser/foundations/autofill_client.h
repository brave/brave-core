// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_AUTOFILL_CORE_BROWSER_FOUNDATIONS_AUTOFILL_CLIENT_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_AUTOFILL_CORE_BROWSER_FOUNDATIONS_AUTOFILL_CLIENT_H_

#define ShowEmailVerifiedToast(...)                                \
  ShowEmailVerifiedToast(__VA_ARGS__);                             \
  virtual void BraveAddSuggestions(                                \
      const PasswordFormClassification& form_classification,       \
      const FormFieldData& field,                                  \
      std::vector<Suggestion>& chromium_suggestions);              \
  virtual bool BraveHandleSuggestion(const Suggestion& suggestion, \
                                     const autofill::FieldGlobalId& field)

#include <components/autofill/core/browser/foundations/autofill_client.h>  // IWYU pragma: export

#undef ShowEmailVerifiedToast

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_AUTOFILL_CORE_BROWSER_FOUNDATIONS_AUTOFILL_CLIENT_H_
