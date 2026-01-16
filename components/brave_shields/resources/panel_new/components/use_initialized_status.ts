/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { useShieldsApi } from '../api/shields_api_context'

export function useInitializedStatus() {
  const api = useShieldsApi()
  const { data: siteBlockInfo } = api.useGetSiteBlockInfo()
  const { data: siteSettings } = api.useGetSiteSettings()
  const { data: browserHeight } = api.useGetBrowserWindowHeight()
  const { data: advancedViewEnabled } = api.useGetAdvancedViewEnabled()

  const initialized = Boolean(
    siteBlockInfo
      && siteSettings
      && browserHeight !== undefined
      && advancedViewEnabled !== undefined,
  )

  React.useEffect(() => {
    if (initialized) {
      api.actions.showUI()
    }
  }, [api, initialized])

  return initialized
}
