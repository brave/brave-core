/* Copyright (c) 2026 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

import * as React from 'react'
import { StateStore } from '$web-common/state_store'

type NullableStateStoreContext<State> = React.Context<StateStore<State> | null>
type MaybeNullableStateStoreContext<State> =
  React.Context<StateStore<State>> | NullableStateStoreContext<State>

/**
 * Creates a custom React hook for subscribing to state changes in a StateStore.
 *
 * @template State - The type of the full state object in the store.
 * @param context - A React Context that provides a StateStore<State> instance.
 * @returns A hook function that accepts a selector and returns the selected
 * state.
 *
 * @example
 * // State and context definitions:
 * interface AppState {
 *   user: { name: string } | null
 * }
 *
 * const AppStoreContext =
 *   React.createContext<StateStore<AppState> | null>(null)
 *
 * // Hook factory usage:
 * const useAppState = createUseStateHook(AppStoreContext)
 *
 * // Hook usage:
 * function UserName() {
 *   const userName = useAppState((state) => state.user?.name ?? 'Guest')
 *   return <span>{userName}</span>
 * }
 */
export function createUseStateHook<State>(
  context: MaybeNullableStateStoreContext<State>,
) {
  function useStateStore<T>(map: (state: State) => T): T {
    // Cast to nullable to handle React.Context type invariance.
    const store = React.useContext(context as NullableStateStoreContext<State>)
    if (!store) {
      throw new Error(
        'useStateStore must be used within a StateStore Provider',
      )
    }
    const [value, setValue] = React.useState(() => map(store.getState()))
    React.useEffect(() => {
      setValue(map(store.getState()))
      return store.addListener((state) => {
        setValue(map(state))
      })
    }, [store])
    return value
  }
  return useStateStore
}
