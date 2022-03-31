/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at http://mozilla.org/MPL/2.0/. */

type Listener<State> = (state: State) => void

export interface StateManager<State> {
  getState: () => State
  update: (source: Partial<State>) => void
  addListener: (listener: Listener<State>) => (() => void)
}

export function createStateManager<State> (
  initialState: State
): StateManager<State> {
  const listeners = new Set<Listener<State>>()
  const state = { ...initialState }

  let sendInitialState = false

  function getState () {
    return state
  }

  function update (source: Partial<State>) {
    for (const [key, value] of Object.entries(source)) {
      if (value !== undefined) {
        state[key] = value
      }
    }
    sendInitialState = false
    for (const listener of listeners) {
      try {
        listener(state)
      } catch (e) {
        // Rethrow error in a new microtask to prevent
        // listeners from interfering with each other
        queueMicrotask(() => { throw e })
      }
    }
  }

  function addListener (listener: Listener<State>) {
    if (!listeners.has(listener)) {
      listeners.add(listener)
      // Send initial state to listeners in a microtask
      sendInitialState = true
      queueMicrotask(() => {
        if (sendInitialState && listeners.has(listener)) {
          listener(state)
        }
      })
    }
    return () => { listeners.delete(listener) }
  }

  return { getState, update, addListener }
}
