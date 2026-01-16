/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { useShieldsApi } from '../api/shields_api_context'

export function useCacheInvalidator() {
  const api = useShieldsApi()

  // Invalidate caches when panel is (re)displayed.
  React.useEffect(() => {
    const listener = () => {
      if (document.visibilityState === 'visible') {
        api.getAdvancedViewEnabled.invalidate()
        api.getBrowserWindowHeight.invalidate()
        api.getSiteBlockInfo.invalidate()
        api.getSiteSettings.invalidate()
        api.repeatedReloadsDetected.invalidate()
        api.areAnyBlockedElementsPresent.invalidate()
        api.actions.updateFavicon()
      }
    }
    document.addEventListener('visibilitychange', listener)
    return () => document.removeEventListener('visibilitychange', listener)
  }, [api])

  // Reset site settings data when the host changes.
  const { data: siteBlockInfo } = api.useGetSiteBlockInfo()
  const [host, setHost] = React.useState(siteBlockInfo?.host ?? '')
  React.useEffect(() => {
    const newHost = siteBlockInfo?.host ?? ''
    if (host && newHost) {
      api.getSiteSettings.reset()
    }
    setHost(newHost)
  }, [siteBlockInfo?.host])
}
