// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler'

mangle((root) => {
  const viewManager = root.getElementById('viewManager')
  if (!viewManager) {
    throw new Error(`[Settings] Autofill page index: couldn't find #viewManager`)
  }
  viewManager.insertAdjacentHTML(
      'beforeend',
      `\${this.isEmailAliasesEnabled_() ? html\`
        <settings-email-aliases-page slot="view" id="email-aliases"
            data-parent-view-id="parent">
        </settings-email-aliases-page>
      \` : ''}`)
})
