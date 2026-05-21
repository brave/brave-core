// Copyright (c) 2026 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

import {
  getDominantColorFromImageURL,
  getDominantColorFromImageURLAsync,
} from '../../../../../../utils/style.utils'

/**
 * Resolves a dominant color from a wallet token logo URL after the image has
 * loaded. Returns undefined until a color is available (show a placeholder
 * meanwhile).
 */
export function useDominantColorFromImageURL(
  src: string | undefined,
  opacity: number = 0.2,
): string | undefined {
  const [color, setColor] = React.useState<string | undefined>(undefined)

  React.useEffect(() => {
    if (!src) {
      setColor(undefined)
      return
    }

    const syncResult = getDominantColorFromImageURL(src, opacity)
    if (syncResult) {
      setColor(syncResult)
      return
    }

    setColor(undefined)
    let cancelled = false
    getDominantColorFromImageURLAsync(src, opacity).then((asyncResult) => {
      if (!cancelled) {
        setColor(asyncResult ?? undefined)
      }
    })

    return () => {
      cancelled = true
    }
  }, [src, opacity])

  return color
}
