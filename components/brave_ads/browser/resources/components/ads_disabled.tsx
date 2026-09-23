/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useRouter } from '../lib/router'
import * as routes from '../lib/app_routes'

// Shown in place of a tab's real content when that tab is hidden from the
// nav because the ads service isn't running, but the route was reached
// directly (e.g. a bookmarked or typed-in URL), so it can't just fall
// through to `NavList`'s own "not found" case.
export function AdsDisabled() {
  const router = useRouter()

  return (
    <div className='content-card'>
      <h4>
        <span className='title'>Ads are disabled</span>
      </h4>
      <p>
        This information isn't available while the ads service isn't
        running. Visit the{' '}
        <a
          href={routes.diagnostics}
          onClick={(event) => {
            event.preventDefault()
            router.setRoute(routes.diagnostics)
          }}
        >
          Diagnostics
        </a>{' '}
        tab to see why.
      </p>
    </div>
  )
}
