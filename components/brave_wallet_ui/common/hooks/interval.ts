// Copyright (c) 2021 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// you can obtain one at https://mozilla.org/MPL/2.0/.
import { useEffect, useLayoutEffect, useRef } from 'react'

function useInterval (callback: () => void, delay: number | null, initialDelay?: number | null) {
  const savedCallback = useRef(callback)

  // Remember the latest callback if it changes.
  useLayoutEffect(() => {
    savedCallback.current = callback
  }, [callback])

  // Set up the interval.
  useEffect(() => {
    // Don't schedule if no delay is specified.
    if (!delay) {
      return
    }

    const id = setInterval(() => savedCallback.current(), initialDelay ?? delay)
    return () => clearInterval(id)
  }, [delay, initialDelay])
}

export default useInterval
