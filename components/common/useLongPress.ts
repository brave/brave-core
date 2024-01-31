// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

const DEFAULT_THRESHOLD = 500

export interface LongPressProps {
  onLongPress: (id?: number) => void
  delay?: number
}

export default function useLongPress(props: LongPressProps) {
  const touchTimer = React.useRef<NodeJS.Timeout | number>()

  const start = React.useCallback((id: number) => {
    touchTimer.current = setTimeout(() => props.onLongPress(id), props.delay === undefined ? DEFAULT_THRESHOLD : props.delay)
  }, [props.onLongPress, props.delay])

  const stop = React.useCallback(() => {
    if (touchTimer.current) {
      clearTimeout(touchTimer.current)
      touchTimer.current = undefined
    }
  }, [])


  return {
    start,
    stop
  }
}
