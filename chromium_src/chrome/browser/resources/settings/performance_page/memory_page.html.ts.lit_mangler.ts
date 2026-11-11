// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler'

// Upstream moved "Always keep these sites active" (the tab-discard exception
// list) off of the memory page and onto its own general performance
// settings page, which we don't show. We want it back, as the first item of
// the Memory section, since our settings-brave-system-page-index stacks the
// Memory page directly under System, making it read as one page. The
// companion memory_page.ts override registers <tab-discard-exception-list>,
// which upstream's memory_page.ts no longer imports itself.
mangle((root) => {
  const section = root.querySelector('settings-section')
  if (!section) {
    throw new Error(
      `[Settings] Memory page: couldn't find settings-section`)
  }

  section.insertAdjacentHTML(
    'afterbegin',
    `<tab-discard-exception-list id="exceptionList">
     </tab-discard-exception-list>
     <div class="hr"></div>`)
})
