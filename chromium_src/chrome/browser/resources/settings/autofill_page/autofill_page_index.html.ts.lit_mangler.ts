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

  // brave_routes.ts deletes the identity docs and travel autofill routes so
  // they can't be navigated to (we don't support those Autofill AI data
  // types). Remove their views too, since this element unconditionally reads
  // `this.routes_.IDENTITY_DOCS.path` / `.TRAVEL.path`, which throws once
  // those routes are gone.
  for (const id of ['identityDocs', 'travel']) {
    const view = root.getElementById(id)
    if (!view) {
      throw new Error(`[Settings] Autofill page index: couldn't find #${id}`)
    }
    view.remove()
  }

  viewManager.insertAdjacentHTML(
      'beforeend',
      `\${this.isEmailAliasesEnabled_() ? html\`
        <settings-email-aliases-page slot="view" id="email-aliases"
            data-parent-view-id="parent">
        </settings-email-aliases-page>
      \` : ''}`)
})
