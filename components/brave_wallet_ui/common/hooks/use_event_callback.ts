// Copyright (c) 2024 The Brave Authors. All rights reserved.
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this file,
// You can obtain one at https://mozilla.org/MPL/2.0/.

import * as React from 'react'

/** creates a memoized event callback. */
export function useEventCallback<Args extends unknown[], R>(
  fn: (...args: Args) => R
): (...args: Args) => R
export function useEventCallback<Args extends unknown[], R>(
  fn: ((...args: Args) => R) | undefined
): ((...args: Args) => R) | undefined
export function useEventCallback<Args extends unknown[], R>(
  fn: ((...args: Args) => R) | undefined
): ((...args: Args) => R) | undefined {
  const ref = React.useRef<typeof fn>(() => {
    throw new Error('Cannot call an event handler while rendering.')
  })

  React.useLayoutEffect(() => {
    ref.current = fn
  }, [fn])

  return React.useCallback(
    (...args: Args) => ref.current?.(...args),
    [ref]
  ) as (...args: Args) => R
}
