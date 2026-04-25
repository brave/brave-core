/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'

interface Opts<T> {
  // The key used to store data in local storage.
  key: string

  // Converts the string value from local storage into a state object of type T.
  parse: (value: string) => T

  // Converts a state object of type T into a string that will be persisted to
  // local storage.
  stringify: (state: T) => string
}

// A React hook that adds component state whose value is persisted to local
// storage. Local storage is queried when the component is first rendered, and
// any update to the state value is written to local storage.
//
// Example:
//
//   const [counter, setCounter] = usePersistedState({
//     key: 'my-persisted-counter',
//     parse: (value) => Number(value) || 0,
//     stringify: (state) => String(state)
//   })
//
// When reading data from local storage, be careful not to assume that the data
// is in any particular format. In particular, be wary of using type assertions
// (e.g. the `as` operator) in the `parse` callback.
//
// For persisted state that is serialized using JSON, see `usePersistedJSON`.
export function usePersistedState<T>(
  opts: Opts<T>
): [T, React.Dispatch<React.SetStateAction<T>>] {
  const [state, setState] = React.useState(() => {
    return opts.parse(localStorage.getItem(opts.key) ?? '')
  })

  React.useEffect(() =>  {
    localStorage.setItem(opts.key, opts.stringify(state))
  }, [state])

  return [state, setState]
}

// A React hook that adds component state whose value is persisted to local
// storage using JSON as the serialization format.
//
//   const [point, setPoint] = usePersistedJSON('my-point', (data: any) => {
//     try {
//       return { x: data.x, y: data.y }
//     } catch {
//       return { x: 0, y: 0 }
//     }
//   })
//
// Be wary of using type assertions (e.g. the `as` operator) in the provided
// callback, as there is no guarantee that the value stored in local storage is
// in any particular format.
export function usePersistedJSON<T>(
  key: string,
  mapData: (data: any) => T
) {
  return usePersistedState<T>({
    key,
    parse: (value) => {
      let data: any = null
      try { data = JSON.parse(value) } catch {}
      return mapData(data)
    },
    stringify: JSON.stringify
  })
}
