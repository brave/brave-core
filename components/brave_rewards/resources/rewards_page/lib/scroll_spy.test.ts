/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import { getActiveSectionRoute } from './scroll_spy'

describe('getActiveSectionRoute', () => {
  const height = 1000

  it('returns null when there are no sections', () => {
    expect(getActiveSectionRoute([], height, 'middle')).toBeNull()
  })

  it('returns the first section at the top of the page', () => {
    const sections = [
      { route: '/', top: 0 },
      { route: '/explore', top: 1800 },
    ]
    expect(getActiveSectionRoute(sections, height, 'middle')).toBe('/')
  })

  it('keeps the first section until the next one nears the top', () => {
    const sections = [
      { route: '/', top: -600 },
      { route: '/explore', top: 700 },
    ]
    expect(getActiveSectionRoute(sections, height, 'middle')).toBe('/')
  })

  it('switches when the next section is within the top 40%', () => {
    const sections = [
      { route: '/', top: -1400 },
      { route: '/explore', top: 400 },
    ]
    expect(getActiveSectionRoute(sections, height, 'middle')).toBe('/explore')
  })

  it('returns the last section when scrolled to the end', () => {
    const sections = [
      { route: '/', top: -300 },
      { route: '/explore', top: 900 },
    ]
    expect(getActiveSectionRoute(sections, height, 'end')).toBe('/explore')
  })

  it('returns the first section at the start even if the next one is near', () => {
    const sections = [
      { route: '/', top: 0 },
      { route: '/explore', top: 300 },
    ]
    expect(getActiveSectionRoute(sections, height, 'middle')).toBe('/explore')
    expect(getActiveSectionRoute(sections, height, 'start')).toBe('/')
  })

  it('returns null when the page cannot scroll', () => {
    const sections = [
      { route: '/', top: 0 },
      { route: '/explore', top: 300 },
    ]
    expect(getActiveSectionRoute(sections, height, 'none')).toBeNull()
  })

  it('handles a single section', () => {
    const sections = [{ route: '/', top: 50 }]
    expect(getActiveSectionRoute(sections, height, 'middle')).toBe('/')
  })
})
