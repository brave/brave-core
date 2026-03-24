/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

import { useShieldsApi } from '../api/shields_api_context'

interface Opts {
  maxResults: number
  maxQueries: number
}

export function useResourceFaviconUrls(urls: string[], opts: Opts) {
  const api = useShieldsApi()
  const [pageUrls, setPageUrls] = React.useState<string[]>([])

  // Gets the first `opts.maxQueries` deduped empty-path URLs from `urls`.
  const queryUrls = React.useMemo(() => {
    const set = new Set<string>()
    for (const url of urls) {
      if (set.size >= opts.maxQueries) {
        break
      }
      try {
        const parsed = new URL(url)
        set.add(`${parsed.protocol}//${parsed.host}/`)
      } catch {}
    }
    return [...set.values()]
  }, [urls.join('\n')])

  React.useEffect(() => {
    let cancelled = false

    // Finds the first `opts.maxResults` favicon page URLs that have a match in
    // the profile's favicon database.
    async function getFaviconUrls() {
      const results = []
      for (const url of queryUrls) {
        if (cancelled || results.length >= opts.maxResults) {
          break
        }
        const { isAvailable } = await api.isResourceFaviconAvailable({ url })
        if (isAvailable) {
          results.push(url)
        }
      }
      return results
    }

    getFaviconUrls().then((pageUrls) => {
      if (!cancelled) {
        setPageUrls(pageUrls)
      }
    })

    return () => {
      cancelled = true
    }
  }, [queryUrls])

  return pageUrls.map(
    (pageUrl: string) =>
      `chrome://favicon2/?size=32&pageUrl=${encodeURIComponent(pageUrl)}`,
  )
}
