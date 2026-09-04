// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { getValidBrowserProfiles, getUniqueBrowserTypes } from './utils'
import { BrowserType } from './component_types'
import { BrowserProfile } from '../api/welcome_browser_proxy'

const makeProfile = (name: string): BrowserProfile => ({
  name,
  index: 0,
  profileName: 'Default',
  history: true,
  favorites: true,
  passwords: false,
  search: false,
  autofillFormData: false,
})

describe('getValidBrowserProfiles', () => {
  it('maps Brave Origin profiles to a distinct browser type', () => {
    const [profile] = getValidBrowserProfiles([makeProfile('Brave Origin Default')])
    // Brave Origin must not collapse into the Brave group; it gets its own type.
    expect(profile.browserType).toBe(BrowserType.Brave_Origin)
    expect(profile.browserType).not.toBe(BrowserType.Brave)
  })

  it('keeps Brave and Brave Origin as separate groups', () => {
    const results = getValidBrowserProfiles([
      makeProfile('Brave Default'),
      makeProfile('Brave Origin Default'),
    ])
    expect(results[0].browserType).toBe(BrowserType.Brave)
    expect(results[1].browserType).toBe(BrowserType.Brave_Origin)
    // The two products form two distinct icon groups.
    expect(getUniqueBrowserTypes(results)).toEqual([
      BrowserType.Brave,
      BrowserType.Brave_Origin,
    ])
  })

  it('maps plain Brave channel profiles to the Brave type', () => {
    const [profile] = getValidBrowserProfiles([makeProfile('Brave Nightly Person 1')])
    expect(profile.browserType).toBe(BrowserType.Brave)
  })

  it('prefers the longest match for prefixed browser names', () => {
    const [profile] = getValidBrowserProfiles([makeProfile('Google Chrome Beta Person 1')])
    expect(profile.browserType).toBe(BrowserType.Chrome_Beta)
  })

  it('filters out the Bookmarks HTML File pseudo-profile', () => {
    const results = getValidBrowserProfiles([
      makeProfile('Bookmarks HTML File'),
      makeProfile('Brave Origin Default'),
    ])
    expect(results).toHaveLength(1)
    expect(results[0].browserType).toBe(BrowserType.Brave_Origin)
  })
})
