/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { useShieldsApi } from '../api/shields_api_context'
import { debounce } from '$web-common/debounce'

export function useVisibilityObserver() {
  const api = useShieldsApi()

  const invalidateCache = React.useMemo(() => {
    return debounce(() => {
      api.getAdvancedViewEnabled.invalidate()
      api.getBrowserWindowHeight.invalidate()
      api.getSiteBlockInfo.invalidate()
      api.getSiteSettings.invalidate()
      api.repeatedReloadsDetected.invalidate()
      api.areAnyBlockedElementsPresent.invalidate()
      api.actions.updateFavicon()
    }, 60)
  }, [api])

  React.useEffect(() => {
    document.addEventListener('visibilitychange', invalidateCache)
    return () => {
      document.removeEventListener('visibilitychange', invalidateCache)
    }
  }, [])
}
