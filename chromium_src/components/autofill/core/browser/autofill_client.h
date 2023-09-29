/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_COMPONENTS_AUTOFILL_CORE_BROWSER_AUTOFILL_CLIENT_H_
#define BRAVE_CHROMIUM_SRC_COMPONENTS_AUTOFILL_CORE_BROWSER_AUTOFILL_CLIENT_H_

#define ConfirmSaveIbanLocally                                                 \
  ConfirmAutocomplete(base::OnceCallback<void(absl::optional<bool>)> callback) \
      const = 0;                                                               \
  virtual void ConfirmSaveIbanLocally

#include "src/components/autofill/core/browser/autofill_client.h"

#undef ConfirmSaveIbanLocally

#endif  // BRAVE_CHROMIUM_SRC_COMPONENTS_AUTOFILL_CORE_BROWSER_AUTOFILL_CLIENT_H_
