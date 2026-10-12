// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler'

mangle(
  (element) => {
    const name = element.querySelector('#name-and-version')
    if (!name) {
      throw new Error(
        '[Brave Extensions Overrides] '
          + 'Could not find element. Has the ID changed?',
      )
    }
    name.insertAdjacentHTML(
      'afterend',
      `<div ?hidden="\${!this.isBraveHosted_(this.data.id)}"
            class="brave-hosted">
          \$i18n{braveHosted}
       </div>`,
    )
  },
  (t) => t.text.includes('id="name-and-version"'),
)

mangle(
  (element) => {
    const warning = element.querySelector('#blocklisted-warning')
    if (!warning) {
      throw new Error(
        '[Brave Extensions Overrides] '
          + 'Could not find #blocklisted-warning. Has the ID changed?',
      )
    }
    warning.insertAdjacentHTML(
      'afterend',
      `<a id="brave-socket-learn-more" target="_blank"
          ?hidden="\${!this.isFlaggedBySocket_(this.data.blocklistText)}"
          href="$i18n{braveBlocklistedMalwareSocketLearnMoreUrl}"
          aria-label="$i18n{braveBlocklistedMalwareSocketLearnMore}">
        $i18n{learnMore}
      </a>`,
    )
  },
  (t) => t.text.includes('id="blocklisted-warning"'),
)
