// Copyright (c) 2022 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'
import { BrowserType } from './component_types'
import { BrowserProfile, ImportDataBrowserProxyImpl } from '../api/import_data_browser'

export const getValidBrowserProfiles = (profiles: BrowserProfile[]) => {
  const browserList = Object.values(BrowserType)

  const getBrowserName = (toFind: string) => {
    // TODO(tali): Add exact matching for cases like "Chrome" vs "Chrome Canary"
    return browserList.find(browser => toFind.includes(browser))
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
  return Array.from(new Set(browsersTypes))
}

export function useInitializeImportData () {
  const [browserProfiles, setProfiles] = React.useState<BrowserProfile[] | undefined>(undefined)

  React.useEffect(() => {
    const fetchAllBrowserProfiles = async () => {
      const res = await ImportDataBrowserProxyImpl.getInstance().initializeImportDialog()
      const validProfiles = getValidBrowserProfiles(res)
      setProfiles(validProfiles)
    }

    fetchAllBrowserProfiles()
  }, [])

  return {
    browserProfiles
  }
}

export function useProfileCount () {
  const profileCountRef = React.useRef(0)

  const incrementCount = () => {
    profileCountRef.current++
  }

  const decrementCount = () => {
    profileCountRef.current--
  }

  return {
    profileCountRef,
    incrementCount,
    decrementCount
  }
}
