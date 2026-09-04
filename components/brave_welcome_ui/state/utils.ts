// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { BrowserProfile } from '../api/welcome_browser_proxy'
import { BrowserType } from './component_types'

const browserList = Object.values(BrowserType)

// Determines the source browser for a detected profile from its display name.
// The C++ importer prepends the browser brand to the profile name, so the
// brand is always at the *start* of the name (e.g. a Chrome profile that was
// previously imported into Brave is detected by Brave Origin as
// "Brave Google Chrome Your Chrome"). We must match against the start of the
// name: a substring match would misclassify that profile as "Google Chrome"
// because its name still contains the original Chrome profile name, causing it
// to appear under the Chrome tile instead of the Brave tile.
// https://github.com/brave/brave-browser/issues/56072
// Note: BrowserType lists the longer Chrome variants (Canary/Beta/Dev) before
// plain "Google Chrome", so startsWith also resolves those correctly.
export const getBrowserType = (profileName: string) => {
  return browserList.find((browser) => profileName.startsWith(browser))
}

export const getUniqueBrowserTypes = (browserProfiles: BrowserProfile[]) => {
  const browsersTypes = browserProfiles.map((profile) => profile.browserType)
  // We use set here to remove dupes and maintain order
  return Array.from(new Set(browsersTypes))
}
