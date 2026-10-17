// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { getBrowserType } from './utils'
import { BrowserType } from './component_types'

describe('getBrowserType', () => {
  it('classifies a regular Chrome profile as Chrome', () => {
    expect(getBrowserType('Google Chrome Your Chrome')).toBe(BrowserType.Chrome)
    expect(getBrowserType('Google Chrome Profile 2')).toBe(BrowserType.Chrome)
  })

  it('matches the longer Chrome variants before plain Chrome', () => {
    expect(getBrowserType('Google Chrome Canary Default'))
      .toBe(BrowserType.Chrome_Canary)
    expect(getBrowserType('Google Chrome Beta Default'))
      .toBe(BrowserType.Chrome_Beta)
    expect(getBrowserType('Google Chrome Dev Default'))
      .toBe(BrowserType.Chrome_Dev)
  })

  // https://github.com/brave/brave-browser/issues/56072
  // A Chrome profile previously imported into Brave keeps the Chrome-derived
  // name; Brave Origin detects it from the Brave user data folder and prepends
  // the "Brave" brand. It must be grouped under Brave, not Chrome.
  it('classifies a Brave profile imported from Chrome as Brave', () => {
    expect(getBrowserType('Brave Google Chrome Your Chrome'))
      .toBe(BrowserType.Brave)
    expect(getBrowserType('Brave Google Chrome Profile 2'))
      .toBe(BrowserType.Brave)
  })

  it('classifies a regular Brave profile as Brave', () => {
    expect(getBrowserType('Brave Default')).toBe(BrowserType.Brave)
  })

  it('does not match a brand that only appears mid-name', () => {
    // A user-chosen Chrome profile name that happens to contain "Brave".
    expect(getBrowserType('Google Chrome Brave stuff')).toBe(BrowserType.Chrome)
  })

  it('returns undefined for an unknown source', () => {
    expect(getBrowserType('Bookmarks HTML File')).toBeUndefined()
  })
})
