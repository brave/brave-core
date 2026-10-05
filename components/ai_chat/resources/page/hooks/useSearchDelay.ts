// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Delays a search query as brave://history's search field does before it
// searches (see cr_search_field_mixin.ts): 500ms after the first character,
// 100ms less for each further one, down to 200ms.
export default function useSearchDelay(query: string) {
  const [delayedQuery, setDelayedQuery] = React.useState(query)

  React.useEffect(() => {
    const delayMs =
      query.length > 0 ? 500 - 100 * (Math.min(query.length, 4) - 1) : 0
    const timer = setTimeout(() => setDelayedQuery(query), delayMs)
    return () => clearTimeout(timer)
  }, [query])

  return delayedQuery
}
