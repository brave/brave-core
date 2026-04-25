// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import { BrowserProfile } from '../api/welcome_browser_proxy'

export const getUniqueBrowserTypes = (browserProfiles: BrowserProfile[]) => {
  const browsersTypes = browserProfiles.map((profile) => profile.browserType)
  // We use set here to remove dupes and maintain order
  return Array.from(new Set(browsersTypes))
}
