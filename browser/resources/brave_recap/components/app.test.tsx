// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { render, screen } from '@testing-library/react'
import { App } from './app'

const DAY_MS = 24 * 60 * 60 * 1000

function renderApp(adsBlocked: number) {
  return render(
    <App
      stats={{
        adsBlocked,
        bandwidthSavedBytes: 5 * 1024 * 1024 * 1024,
        firstRunTime: Date.now() - 731 * DAY_MS,
      }}
    />,
  )
}

describe('brave recap app', () => {
  it('shows the empty state below 100 blocked items', () => {
    renderApp(99)
    expect(screen.getByRole('heading', { level: 1 })).toHaveTextContent(
      'BRAVE_RECAP_EMPTY_HEADING',
    )
  })

  it('shows the intro once enough has been blocked', () => {
    renderApp(100)
    expect(screen.getByRole('heading', { level: 1 })).toHaveTextContent(
      'BRAVE_RECAP_INTRO_HEADING',
    )
    expect(screen.getByText('BRAVE_RECAP_PRIVACY_NOTE')).toBeInTheDocument()
  })
})
