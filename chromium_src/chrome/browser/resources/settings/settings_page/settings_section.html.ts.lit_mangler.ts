// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler'

// Upstream only hides the header of a section with no title and no feedback
// button under Settings Refresh 2026, which we don't enable. Hide it always so
// an untitled section doesn't show an empty header. The always-true flag check
// keeps the property referenced, which upstream's lit-member-visibility lint
// requires for a protected member.
mangle(
  (root) => {
    const header = root.querySelector('#header')
    if (!header) {
      throw new Error(`[Settings] settings-section: couldn't find #header`)
    }
    header.removeAttribute('?hidden')
    // '?' isn't valid in setAttribute() names, so splice the Lit binding into
    // the markup. Re-parsing replaces the element, so this goes last. The
    // template is shipped with the browser: nothing to sanitize.
    const openingTagNameEnd = header.tagName.length + 1
    const outerHtml = header.outerHTML
    // eslint-disable-next-line no-unsanitized/property
    header.outerHTML = outerHtml.slice(0, openingTagNameEnd)
      + ' ?hidden="${this.settingsRefresh2026Enabled_ !== null &&'
      + ' !this.pageTitle && !this.showSendFeedbackButton}"'
      + outerHtml.slice(openingTagNameEnd)
  },
  (template) => template.text.includes('id="header"'),
)
