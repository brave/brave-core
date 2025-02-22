/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#include "components/autofill/core/browser/ui/autofill_external_delegate.h"

#define kManagePlusAddress \
  kNewEmailAlias:          \
  case SuggestionType::kManagePlusAddress

#include "src/components/autofill/core/browser/ui/autofill_external_delegate.cc"

#undef kManagePlusAddress
