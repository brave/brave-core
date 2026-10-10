// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { BrowserProfile } from '../api/welcome_browser_proxy'
import { BrowserType } from './component_types'

const browserList = Object.values(BrowserType)

export const getValidBrowserProfiles = (profiles: BrowserProfile[]) => {
  const getBrowserName = (toFind: string) => {
    // Prefer the longest matching browser name so more specific products win
    // over their prefixes, e.g. "Brave Origin" over "Brave" and
    // "Google Chrome Beta" over "Google Chrome".
    return browserList
      .filter(browser => toFind.includes(browser))
      .sort((a, b) => b.length - a.length)[0]
  }

  let results = profiles
    .filter((profile) => profile.name !== 'Bookmarks HTML File')
    .map((profile) => {
      const browserType = getBrowserName(profile.name)
      // Introducing a new property here
      return { ...profile, browserType }
    })

  return results
}

export const getUniqueBrowserTypes = (browserProfiles: BrowserProfile[]) => {
  const browsersTypes = browserProfiles.map((profile) => profile.browserType)
  // We use set here to remove dupes and maintain order
  return Array.from(new Set(browsersTypes))
}
