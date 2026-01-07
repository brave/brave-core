/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { loadTimeData } from '$web-common/loadTimeData'

const newsFeatureEnabled = loadTimeData.getBoolean('newsFeatureEnabled')

type ProviderComponent = React.ComponentType<{ children: React.ReactNode }>

// Cache the loaded provider at module level
let cachedProvider: ProviderComponent | null = null
let loadPromise: Promise<ProviderComponent | null> | null = null

function loadProvider(): Promise<ProviderComponent | null> {
  if (!newsFeatureEnabled) {
    return Promise.resolve(null)
  }
  if (!loadPromise) {
    loadPromise = import(
      '../../../../components/brave_news/browser/resources/shared/Context'
    ).then((m) => {
      cachedProvider = m.BraveNewsContextProvider
      return cachedProvider
    })
  }
  return loadPromise
}

export function NewsProvider(props: React.PropsWithChildren) {
  const [Provider, setProvider] = React.useState<ProviderComponent | null>(
    cachedProvider,
  )

  React.useEffect(() => {
    if (!Provider && newsFeatureEnabled) {
      loadProvider().then((p) => {
        if (p) setProvider(() => p)
      })
    }
  }, [Provider])

  if (Provider) {
    return <Provider>{props.children}</Provider>
  }

  return <>{props.children}</>
}
