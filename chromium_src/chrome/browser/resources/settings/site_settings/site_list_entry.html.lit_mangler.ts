// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler'

// Exceptions that Shields or Google sign-in control can't be reset here: show
// the reset button disabled, with a tooltip saying why. The bound methods come
// from browser/resources/settings/br/site_list_entry.ts.
mangle(
  (root) => {
    const resetSite = root.querySelector('#resetSite')
    if (!resetSite) {
      throw new Error(`[Settings] site-list-entry: couldn't find #resetSite`)
    }
    resetSite.removeAttribute('class')
    resetSite.setAttribute('iron-icon', 'trash')
    // '?' and '@' aren't valid in setAttribute() names, so splice the Lit
    // bindings into the markup. Re-parsing replaces the element, so this goes
    // last. The template is shipped with the browser: nothing to sanitize.
    const openingTagNameEnd = resetSite.tagName.length + 1
    const outerHtml = resetSite.outerHTML
    // eslint-disable-next-line no-unsanitized/property
    resetSite.outerHTML = outerHtml.slice(0, openingTagNameEnd)
      + ' ?disabled="${this.shouldDisableReset_()}"'
      + ' @mouseenter="${this.onResetSiteMouseenter_}"'
      + outerHtml.slice(openingTagNameEnd)
  },
  (template) => template.text.includes('id="resetSite"'),
)
