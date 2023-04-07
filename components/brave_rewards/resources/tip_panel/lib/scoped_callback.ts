/* Copyright (c) 2023 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

// A hook useful for integrating async APIs with component state.
//
// When setting up a promise callback from a component event handler, it might
// turn out that the component is dismounted before the promise resolves. In
// such a case, you may see the following warning:
//
//   "Can't perform a React state update on an unmounted component."
//
// By wrapping callbacks with the function returned by `useScopedCallback`, the
// wrapped callback will not be executed after the component has dismounted.
//
export function useScopedCallback () {
  const active = React.useRef(true)
  React.useEffect(() => {
    return () => { active.current = false }
  }, [])
  return <T extends unknown[], R>(callback: (...args: [...T]) => R) => {
    return (...args: [...T]) => {
      if (active.current) {
        return callback(...args)
      } else {
        return undefined
      }
    }
  }
}
