/* Copyright (c) 2025 The Brave Authors. All rights reserved.
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this file,
 * You can obtain one at https://mozilla.org/MPL/2.0/. */

type Listener<State> = (state: State) => void

type UpdateFunction<State> = (state: State) => Partial<State>

// A simple object-state store.
export interface Store<State> {

  // Returns the current state of the store.
  getState: () => State

  // Updates the state of the store. All listeners will be notified of the state
  // change in a future turn of the event loop.
  update: (source: Partial<State> | UpdateFunction<State>) => void

  // Adds a listener that will be notified when the state store changes. The
  // listener will not be notified immediately. Returns a function that will
  // remove the listener from store.
  addListener: (listener: Listener<State>) => (() => void)

}

export function createStore<State>(initialState: State): Store<State> {
  const listeners = new Set<Listener<State>>()
  const state = { ...initialState }
  let notificationQueued = false

  function notify() {
    if (notificationQueued) {
      return
    }

    notificationQueued = true

    // Send update notifications in a future turn in order to avoid reentrancy.
    queueMicrotask(() => {
      notificationQueued = false
      for (const listener of listeners) {
        // If a notification has been queued as a result of calling a listener,
        // then exit this notification. The next update will be sent in a future
        // turn to all remaining listeners.
        if (notificationQueued) {
          break
        }
        try {
          listener(state)
        } catch (e) {
          // Rethrow error in a future turn to prevent listeners from
          // interfering with each other.
          queueMicrotask(() => { throw e })
        }
      }
    })
  }

  return {

    getState() {
      return state
    },

    update(source: Partial<State> | UpdateFunction<State>) {
      if (typeof source === 'function') {
        source = source(state)
      }
      for (const [key, value] of Object.entries(source)) {
        if (value !== undefined) {
          (state as any)[key] = value
        }
      }
      notify()
    },

    addListener(listener: Listener<State>) {
      if (!listeners.has(listener)) {
        listeners.add(listener)
      }
      return () => {
        listeners.delete(listener)
      }
    }

  }
}
