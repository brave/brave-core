// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { act, fireEvent, render, screen } from '@testing-library/react'
import { App } from './app'

const DAY_MS = 24 * 60 * 60 * 1000

const GB = 1024 * 1024 * 1024

function renderApp(
  adsBlocked: number,
  bandwidthSavedBytes = 5 * GB,
  firstRunTime = Date.now() - 731 * DAY_MS,
) {
  return render(
    <App stats={{ adsBlocked, bandwidthSavedBytes, firstRunTime }} />,
  )
}

function heading(level: number) {
  return screen.getByRole('heading', { level })
}

function pressKey(key: string) {
  fireEvent.keyDown(window, { key })
}

// Leo buttons ignore fireEvent's synthetic clicks, so use a native one.
function click(text: string) {
  act(() => screen.getByText(text).click())
}

// Skips the count-up animation so slides show their final values.
function preferReducedMotion() {
  jest.spyOn(window, 'matchMedia').mockImplementation(
    (query) =>
      ({
        matches: query === '(prefers-reduced-motion: reduce)',
        media: query,
      }) as MediaQueryList,
  )
}

describe('brave recap app', () => {
  afterEach(() => jest.restoreAllMocks())

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

  it('shows the install month and days on the intro when known', () => {
    renderApp(100)
    expect(screen.getByText('BRAVE_RECAP_INTRO_SINCE')).toBeInTheDocument()
    expect(screen.getByText('BRAVE_RECAP_INTRO_DAYS')).toBeInTheDocument()
  })

  it('falls back to generic intro text without an install date', () => {
    renderApp(100, 0, 0)
    expect(screen.getByText('BRAVE_RECAP_INTRO_NO_DATE')).toBeInTheDocument()
  })

  it('moves between slides with the buttons and keyboard', () => {
    renderApp(100)
    click('BRAVE_RECAP_NEXT')
    expect(heading(2)).toHaveTextContent('BRAVE_RECAP_BLOCKED_HEADING')

    pressKey('ArrowRight')
    expect(heading(2)).toHaveTextContent('BRAVE_RECAP_TIME_HEADING')

    pressKey('ArrowLeft')
    click('BRAVE_RECAP_BACK')
    expect(heading(1)).toHaveTextContent('BRAVE_RECAP_INTRO_HEADING')
  })

  it('starts over from the last slide', () => {
    renderApp(100)
    while (screen.queryByText('BRAVE_RECAP_NEXT')) {
      pressKey('ArrowRight')
    }
    click('BRAVE_RECAP_START_OVER')
    expect(heading(1)).toHaveTextContent('BRAVE_RECAP_INTRO_HEADING')
  })

  it('shows the blocked count', () => {
    preferReducedMotion()
    renderApp(1_284_213)
    pressKey('ArrowRight')
    expect(screen.getAllByText('1,284,213').length).toBeGreaterThan(0)
  })

  it('compares time saved to movies', () => {
    const timeText = (adsBlocked: number) => {
      const { unmount } = renderApp(adsBlocked)
      pressKey('ArrowRight')
      pressKey('ArrowRight')
      const text = heading(2).parentElement!.nextElementSibling!.textContent
      unmount()
      return text
    }
    // 144,000 items at 50ms each is one two-hour movie.
    expect(timeText(100)).toBe('BRAVE_RECAP_TIME_TEXT_SHORT')
    expect(timeText(144_000)).toBe('BRAVE_RECAP_TIME_TEXT_ONE')
    expect(timeText(1_284_213)).toBe('BRAVE_RECAP_TIME_TEXT')
  })

  it('compares data saved to hours of HD video', () => {
    const dataText = (bandwidthSavedBytes: number) => {
      const { unmount } = renderApp(100, bandwidthSavedBytes)
      pressKey('ArrowRight')
      pressKey('ArrowRight')
      pressKey('ArrowRight')
      const text = heading(2).parentElement!.nextElementSibling!.textContent
      unmount()
      return text
    }
    expect(dataText(GB)).toBe('BRAVE_RECAP_DATA_TEXT_SHORT')
    expect(dataText(3 * GB)).toBe('BRAVE_RECAP_DATA_TEXT_ONE')
    expect(dataText(9 * GB)).toBe('BRAVE_RECAP_DATA_TEXT')
  })
})
