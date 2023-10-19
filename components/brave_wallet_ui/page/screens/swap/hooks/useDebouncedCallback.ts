// Copyright (c) 2023 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import { useEffect, useRef, useCallback } from 'react'

export function useDebouncedCallback<A extends any[]>(
  callback: ((...args: A) => Promise<void>) | ((...args: A) => void),
  wait: number
) {
  // Track args & timeout handle between calls
  const argsRef = useRef<A>()
  const timeout = useRef<ReturnType<typeof setTimeout>>()

  const cleanup = useCallback(() => {
    if (timeout.current) {
      clearTimeout(timeout.current)
    }
  }, [timeout])

  // Make sure our timeout gets cleared if our consuming component gets
  // unmounted.
  useEffect(() => cleanup, [cleanup])

  return useCallback(
    async function debouncedCallback(...args: A) {
      // capture latest args
      argsRef.current = args

      // clear debounce timer
      cleanup()

      // start waiting again
      timeout.current = setTimeout(async () => {
        if (argsRef.current) {
          await callback(...argsRef.current)
        }
      }, wait)
    },
    [argsRef, timeout, cleanup, callback, wait]
  )
}
