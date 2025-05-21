/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

interface Props {
  onVisible: () => void
  rootMargin?: string
}

export function VisibilityTracker(props: Props) {
  const ref = React.useRef<HTMLDivElement>(null)

  React.useEffect(() => {
    if (!ref.current) {
      return
    }

    const observer = new IntersectionObserver(
      (entries) => {
        for (const entry of entries) {
          if (entry.intersectionRatio > 0) {
            props.onVisible()
            return
          }
        }
      },
      { rootMargin: props.rootMargin || '0px 0px 0px 0px' },
    )

    observer.observe(ref.current)
    return () => {
      observer.disconnect()
    }
  }, [props.onVisible, props.rootMargin])

  return <div ref={ref} />
}
