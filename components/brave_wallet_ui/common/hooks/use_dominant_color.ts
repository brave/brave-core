// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

// Utils
import { getDominantColorFromImageURL } from '../../utils/style.utils'

export const useDominantColor = (logoUrl: string | undefined) => {
  const [color, setColor] = React.useState<string | undefined>(undefined)

  React.useEffect(() => {
    let cancelled = false
    const src = logoUrl ?? ''

    if (!src) {
      setColor(undefined)
      return
    }

    getDominantColorFromImageURL(src).then((result) => {
      if (!cancelled) {
        setColor(result)
      }
    })

    return () => {
      cancelled = true
    }
  }, [logoUrl])

  return color
}
