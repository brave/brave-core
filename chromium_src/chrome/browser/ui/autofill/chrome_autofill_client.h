/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

#ifndef BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_AUTOFILL_CHROME_AUTOFILL_CLIENT_H_
#define BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_AUTOFILL_CHROME_AUTOFILL_CLIENT_H_

#include "components/autofill/core/browser/autofill_client.h"

#define ConfirmSaveIbanLocally                                                 \
  ConfirmAutocomplete(base::OnceCallback<void(absl::optional<bool>)> callback) \
      const override;                                                          \
  void ConfirmSaveIbanLocally

#include "src/chrome/browser/ui/autofill/chrome_autofill_client.h"

#undef ConfirmSaveIbanLocally

#endif  // BRAVE_CHROMIUM_SRC_CHROME_BROWSER_UI_AUTOFILL_CHROME_AUTOFILL_CLIENT_H_
