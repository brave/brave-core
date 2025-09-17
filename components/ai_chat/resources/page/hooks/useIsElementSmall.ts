// Copyright (c) 2025 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

export const useIsElementSmall = (small = 768) => {
  const [isSmall, setIsSmall] = React.useState(false)
  const [element, setElement] = React.useState<HTMLElement | null>(null)

  React.useEffect(() => {
    if (!element) return
    const resizeObserver = new ResizeObserver((entries) => {
      setIsSmall(entries[0].contentRect.width < small)
    })
    resizeObserver.observe(element)
    return () => resizeObserver.disconnect()
  }, [element, small])

  return {
    isSmall,
    setElement,
  }
}
