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
        api.getSiteBlockInfo.invalidate()
        api.getSiteSettings.invalidate()
        api.repeatedReloadsDetected.invalidate()
        api.areAnyBlockedElementsPresent.invalidate()
        api.updateFavicon()
      }
    }
    document.addEventListener('visibilitychange', listener)
    return () => document.removeEventListener('visibilitychange', listener)
  }, [api])
}
