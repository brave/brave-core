// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { mangle } from 'lit_mangler'

mangle((root) => {
  // Shields exceptions take a plain host, so the wildcard example upstream
  // shows for content settings doesn't apply.
  const site = root.getElementById('site')
  if (!site) {
    throw new Error(`[Settings] Add site dialog: couldn't find #site`)
  }
  site.setAttribute(
    'placeholder',
    "${this.category === 'braveShields' ? " +
      "'$i18n{braveShieldsExampleTemplate}' : " +
      "'$i18n{addSiteExceptionPlaceholder}'}")
})
