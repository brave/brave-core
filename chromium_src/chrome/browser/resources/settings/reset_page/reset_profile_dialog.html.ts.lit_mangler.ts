// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler'

// Brave doesn't report settings on reset, so uncheck, disable and hide the
// "report current settings" checkbox along with its footer. It's kept in the
// DOM because upstream's reset_profile_dialog.ts reads it.
mangle((root) => {
  const sendSettings = root.querySelector('#sendSettings')
  if (!sendSettings?.parentElement) {
    throw new Error(`[Settings] Reset profile dialog: couldn't find #sendSettings`)
  }
  sendSettings.removeAttribute('checked')
  sendSettings.setAttribute('disabled', '')
  sendSettings.parentElement.setAttribute('hidden', '')
})
